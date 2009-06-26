/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <es/handle.h>
#include <es/base/IFile.h>
#include <es/base/IProcess.h>
#include <es/naming/IContext.h>
#include "esjs.h"
#include "parser.h"

//
// yacc declarations
//

#ifdef __cplusplus
extern "C" {
#endif

int yyparse();

extern FILE* yyin;

#ifdef __cplusplus
}
#endif

int yylex(YYSTYPE* yylval);

namespace
{
    FormalParameterList* parseFormalParameterList()
    {
        FormalParameterList* arguments = new FormalParameterList;
        YYSTYPE yylval;
        while (int token = yylex(&yylval))
        {
            if (token == IDENTIFIER)
            {
                arguments->add(yylval.identifier);
                token = yylex(&yylval);
                if (token == ',')
                {
                    continue;
                }
                if (token == 0)
                {
                    break;
                }
            }
            // Syntax Error
            switch (token)
            {
            case NUMERIC_LITERAL:
            case STRING_LITERAL:
                delete yylval.expression;
                break;
            case IDENTIFIER:
                delete yylval.identifier;
                break;
            }
            delete arguments;
            return 0;
        }
        return arguments;
    }
}

//
// Function Methods
//

class FunctionMethod : public Code
{
    enum Method
    {
        ToString,
        Apply,
        Call,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* toString()
    {
        std::string s;

        ObjectValue* object = dynamic_cast<ObjectValue*>(getThis());
        if (!object || !object->getCode())
        {
            throw getErrorInstance("TypeError");
        }

        s = "function ";
        Identifier* identifier = object->getIdentifier();
        if (identifier)
        {
            s += identifier->toString();
        }
        s += "(";
        FormalParameterList* formalParameterList = object->getParameterList();
        s += formalParameterList->toString("");
        s += ")\n";
        s += "{\n";
        Code* code = object->getCode();
        s += code->toString("    ");
        s += "}";

        return new StringValue(s);
    }

    Value* apply()
    {
        ObjectValue* object = dynamic_cast<ObjectValue*>(getThis());
        if (!object || !object->getCode())
        {
            throw getErrorInstance("TypeError");
        }

        Value* thisArg = getScopeChain()->get("thisArg");
        if (thisArg->isNull() || thisArg->isUndefined())
        {
            thisArg = getGlobal();
        }

        Register<ListValue> list;
        Value* argArray = getScopeChain()->get("argArray");
        if (argArray->isNull() || argArray->isUndefined())
        {
            list = new ListValue;
        }
        else if (dynamic_cast<ListValue*>(argArray))
        {
            list = static_cast<ListValue*>(argArray);
        }
        else if (dynamic_cast<ArrayValue*>(argArray))
        {
            list = new ListValue;
            ArrayValue* arguments = static_cast<ArrayValue*>(argArray);
            u32 length = arguments->get("length")->toUint32();
            for (u32 i = 0; i < length; ++i)
            {
                char name[12];

                sprintf(name, "%u", i);
                list->push(arguments->get(name));
            }
        }
        else
        {
            throw getErrorInstance("TypeError");
        }

        return object->call(thisArg, list);
    }

    Value* call()
    {
        ObjectValue* object = dynamic_cast<ObjectValue*>(getThis());
        if (!object || !object->getCode())
        {
            throw getErrorInstance("TypeError");
        }

        Value* thisArg = getScopeChain()->get("thisArg");
        if (thisArg->isNull() || thisArg->isUndefined())
        {
            thisArg = getGlobal();
        }

        Register<ListValue> list = new ListValue;
        ListValue* arguments = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        for (int i = 1; i < arguments->length(); ++i)
        {
            list->push((*arguments)[i]);
        }

        return object->call(thisArg, list);
    }

public:
    FunctionMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case Apply:
            arguments->add(new Identifier("thisArg"));
            arguments->add(new Identifier("argArray"));
            break;;
        case Call:
            arguments->add(new Identifier("thisArg"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~FunctionMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Value* value = getThis();

        switch (method)
        {
        case ToString:
            value = toString();
            break;
        case Apply:
            value = apply();
            break;
        case Call:
            value = call();
            break;
        default:
            break;
        }
        return CompletionType(CompletionType::Return, value, "");
    }

    const char* name() const
    {
        return names[method];
    }

    static int methodCount()
    {
        return MethodCount;
    }
};

const char* FunctionMethod::names[] =
{
    "toString",
    "apply",
    "call"
};

//
// Function Prototype
//

class FunctionPrototype : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Object.prototype

public:
    FunctionPrototype(ObjectValue* object) :
        arguments(new FormalParameterList),
        prototype(0)    // Must set to Object.prototype later.
    {
        object->setParameterList(arguments);
    }
    ~FunctionPrototype()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        return CompletionType(CompletionType::Return, UndefinedValue::getInstance(), "");
    }
};

//
// Function Constructor
//

class FunctionConstructor : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Function.prototype

public:
    FunctionConstructor(ObjectValue* function) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        prototype->setCode(new FunctionPrototype(prototype));
        prototype->put("constructor", function);
        for (int i = 0; i < FunctionMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            FunctionMethod* method = new FunctionMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
        function->put("prototype", prototype);
        function->setPrototype(prototype);
    }
    ~FunctionConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        std::string body;
        std::string p;
        if (list->length() == 0)
        {
            body = "";
        }
        else if (list->length() == 1)
        {
            body = (*list)[0]->toString();
        }
        else
        {
            body = (*list)[list->length() - 1]->toString();
            p = (*list)[0]->toString();
            for (int k = 1; k < list->length() - 1; ++k)
            {
                p += ",";
                p += (*list)[k]->toString();
            }
        }

#ifdef VERBOSE
        report("param: '%s'\n", p.c_str());
        report("body: '%s'\n", body.c_str());
#endif

        setSource(p);
        FormalParameterList* formalParameterList = parseFormalParameterList();
        if (!formalParameterList)
        {
            throw getErrorInstance("SyntaxError");
        }

        setSource(body);
        yyparse();
        SourceElements* program = getProgram();

#ifdef VERBOSE
        program->print();
#endif

        Register<ObjectValue> function = new ObjectValue();
        function->setMortal();
        function->setParameterList(formalParameterList);
        function->setCode(program);
        function->setPrototype(prototype);
        function->setScope(getScopeChain());
        Register<Value> prototype = new ObjectValue;
        function->put("prototype", prototype);

        return CompletionType(CompletionType::Return, function, "");
    }
};

ObjectValue* constructFunctionConstructor()
{
    ObjectValue* function = new ObjectValue;
    function->setCode(new FunctionConstructor(function));
    return function;
}
