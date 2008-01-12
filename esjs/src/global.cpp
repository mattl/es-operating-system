/*
 * Copyright (c) 2006, 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <es/handle.h>
#include <es/base/IFile.h>
#include <es/base/IProcess.h>
#include <es/naming/IContext.h>
#include "esjs.h"
#include "parser.h"
#include "interface.h"

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
    ObjectValue*    global;
}

//
// Error Object Value
//

class ErrorValue : public ObjectValue
{
public:
    const char* getClass() const
    {
        return "Error";
    }
};

//
// Error Methods
//

class ErrorMethod : public Code
{
    enum Method
    {
        ToString,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    ErrorMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }

    ~ErrorMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<Value> value;
        std::string s;
        switch (method)
        {
        case ToString:
            s = getThis()->get("message")->toString();
            if (s == "")
            {
                value = getThis()->get("name");
            }
            else
            {
                s = getThis()->get("name")->toString() + ": " + s ;
                value = new StringValue(s);
            }
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

const char* ErrorMethod::names[] =
{
    "toString"
};

//
// Error Constructor
//

class ErrorConstructor : public Code
{
    StringValue*            name;
    FormalParameterList*    arguments;
    ErrorValue*             prototype;  // Error.prototype

public:
    ErrorConstructor(ObjectValue* error, const char* name, ObjectValue* function) :
        name(new StringValue(name)),
        arguments(new FormalParameterList),
        prototype(new ErrorValue)
    {
        arguments->add(new Identifier("message"));
        prototype->put("name", this->name);
        prototype->put("constructor", error);
        if (strcmp(name, "Error") == 0)
        {
            prototype->setPrototype(function->getPrototype()->getPrototype());
            for (int i = 0; i < ErrorMethod::methodCount(); ++i)
            {
                ObjectValue* function = new ObjectValue;
                ErrorMethod* method = new ErrorMethod(function, i);
                function->setCode(method);
                prototype->put(method->name(), function);
            }
        }
        else
        {
            prototype->setPrototype(getGlobal()->get("Error")->get("prototype"));
        }

        error->setParameterList(arguments);
        error->setScope(global);
        error->put("prototype", prototype);
        error->setPrototype(function->getPrototype());
    }
    ~ErrorConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Value* message = getScopeChain()->get("message");
        Register<ErrorValue> object = new ErrorValue;
        if (!message->isUndefined())
        {
            Register<Value> value = new StringValue(message->toString());
            object->put("message", value);
        }
        object->setPrototype(prototype);
        return CompletionType(CompletionType::Return, object, "");
    }
};

Value* getErrorInstance(const char* name, const char* message)
{
    Register<Value> error = global->get(name);
    Register<ListValue> list = new ListValue;
    if (message)
    {
        Register<Value> value = new StringValue(message);
        list->push(value);
    }
    Register<ObjectValue> function = dynamic_cast<ObjectValue*>(error->getValue());
    if (!function || !function->getCode())
    {
        return UndefinedValue::getInstance();
    }
    return function->call(NullValue::getInstance(), list);
}

//
// Global Ojbect
//

class GlobalMethod : public Code
{
    enum Method
    {
        Eval,
        ParseInt,
        ParseFloat,
        IsNaN,
        IsFinite,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

public:
    GlobalMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);

        switch (method)
        {
        case Eval:
            arguments->add(new Identifier("x"));
            break;
        case ParseInt:
            arguments->add(new Identifier("string"));
            arguments->add(new Identifier("radix"));
            break;
        case ParseFloat:
            arguments->add(new Identifier("string"));
            break;
        case IsNaN:
        case IsFinite:
            arguments->add(new Identifier("number"));
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }

    ~GlobalMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<Value> value;
        double          number;
        std::string     s;
        const char*     p;
        char*           end;
        int             base;
        long            integer;
        Value*          x;
        CompletionType  result;
        SourceElements* program;

        switch (method)
        {
        case Eval:
            x = getScopeChain()->get("x");
            if (!x->isString())
            {
                value = x;
                break;
            }

            s = x->toString();
            setSource(s.c_str());
            yyparse();
            program = getProgram();

#ifdef VERBOSE
            program->print();
#endif

            try
            {
                result = program->evaluate();
            }
            catch (Value* value)
            {
                result.setType(CompletionType::Throw);
                result.setValue(value);
            }

            delete program;

            if (result.isNormal())
            {
                if (result.getValue())
                {
                    value = result.getValue();
                }
                else
                {
                    value = UndefinedValue::getInstance();
                }
                Value::sweep(true);
            }
            else
            {
                ASSERT(result.isThrow());
                value = result.getValue();
                Value::sweep(true);
                throw result.getValue();
            }
            break;
        case ParseInt:
            x = getScopeChain()->get("radix");
            if (x->isUndefined())
            {
                base = 0;
            }
            else
            {
                base = (int) (x->toNumber());
                if (base < 2 || 36 < base)
                {
                    value = new NumberValue(NAN);
                    break;
                }
            }

            s = getScopeChain()->get("string")->toString();
            p = s.c_str();
            integer = strtol(p, &end, base);
            if (p == end)
            {
                value = new NumberValue(NAN);
            }
            else
            {
                value = new NumberValue(integer);
            }
            break;
        case ParseFloat:
            s = getScopeChain()->get("string")->toString();
            p = s.c_str();
            p = skipSpace(p);
            number = strtod(p, &end);
            if (number == 0.0 && p == end)
            {
                value = new NumberValue(NAN);
            }
            else
            {
                value = new NumberValue(number);
            }
            break;
        case IsNaN:
            number = getScopeChain()->get("number")->toNumber();
            value = BoolValue::getInstance(isnan(number) ? true : false);
            break;
        case IsFinite:
            number = getScopeChain()->get("number")->toNumber();
            value = BoolValue::getInstance(isfinite(number) ? true : false);
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

const char* GlobalMethod::names[] =
{
    "eval",
    "parseInt",
    "parseFloat",
    "isNaN",
    "isFinite"
};

void constructGlobalObject()
{
    global = new ObjectValue;

    global->put("undefined", UndefinedValue::getInstance());
    global->put("NaN", new NumberValue(NAN));
    global->put("Infinity", new NumberValue(INFINITY));

    // Register function properties of the Global object
    for (int i = 0; i < GlobalMethod::methodCount(); ++i)
    {
        ObjectValue* function = new ObjectValue;
        GlobalMethod* method = new GlobalMethod(function, i);
        function->setCode(method);
        global->put(method->name(), function);
    }

    // Register Function
    ObjectValue* function = constructFunctionConstructor();
    global->put("Function", function);

    // Register Object
    ObjectValue* object = constructObjectConstructor();
    global->put("Object", object);

    function->getPrototype()->setPrototype(object->get("prototype"));

    // Register Error objects
    ObjectValue* error;

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "Error", function));
    global->put("Error", error);

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "EvalError", function));
    global->put("EvalError", error);

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "RangeError", function));
    global->put("RangeError", error);

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "ReferenceError", function));
    global->put("ReferenceError", error);

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "SyntaxError", function));
    global->put("SyntaxError", error);

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "TypeError", function));
    global->put("TypeError", error);

    error = new ObjectValue;
    error->setCode(new ErrorConstructor(error, "URIError", function));
    global->put("URIError", error);

    // Register String object
    ObjectValue* string = constructStringObject();
    global->put("String", string);

    // Register Math object
    ObjectValue* math = constructMathObject();
    global->put("Math", math);

    // Register Array object
    ObjectValue* array = constructArrayObject();
    global->put("Array", array);

    // Register Boolean object
    ObjectValue* boolean = constructBooleanObject();
    global->put("Boolean", boolean);

    // Register RegExp object
    ObjectValue* regexp = constructRegExpObject();
    global->put("RegExp", regexp);

    // Register Date object
    ObjectValue* date = constructDateObject();
    global->put("Date", date);

    // Register Interface object
    global->put("Interface", constructInterfaceObject());

    // Register System object
    global->put("System", constructSystemObject(0));

    // Register Number object
    global->put("Number", constructNumberObject());
}

ObjectValue* getGlobal()
{
    return global;
}
