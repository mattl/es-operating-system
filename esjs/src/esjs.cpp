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

extern ICurrentProcess* System();

int yylex(YYSTYPE* yylval);

#ifdef __cplusplus
extern "C" {
#endif

int yyparse();

extern FILE* yyin;

#ifdef __cplusplus
}
#endif

namespace
{
    SourceElements* program;
    ObjectValue*    global;
}

int stdLex(char* buffer, int max_size)
{
    int len;
    for (len = 0; len < max_size; ++len)
    {
        int c = fgetc(yyin);
        if (c == EOF)
        {
            break;
        }
        *buffer++ = (char) c;
    }
    return len;
}

static std::string source;
static long        lexptr = -1;

int strLex(char* buffer, int max_size)
{
    int len;
    for (len = 0; lexptr < source.length() && len < max_size; ++len)
    {
        *buffer++ = source[lexptr++];
    }
    return len;
}

static IStream* lexStream;

int streamLex(char* buffer, int max_size)
{
    int len;
    for (len = 0; len < max_size; ++len)
    {
        char c;
        int n = lexStream->read(&c, 1);
        if (n == 0)
        {
            break;
        }
        *buffer++ = c;
    }
    return len;
}

int (*getLex())(char* buffer, int max_size)
{
    if (0 <= lexptr)
    {
        return strLex;
    }
    if (lexStream)
    {
        return streamLex;
    }
    return stdLex;
}

void setSource(const std::string& s)
{
    source = s;
    lexptr = 0;
    lexStream = 0;
}

void setSource(IStream* stream)
{
    lexStream = stream;
}

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

//
// Object Constructor
//

class ObjectConstructor : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // Object.prototype

public:
    ObjectConstructor(ObjectValue* object) :
        arguments(new FormalParameterList)
    {
        arguments->add(new Identifier("value"));

        object->setParameterList(arguments);
        object->setScope(global);

        // Create Object.prototype
        Register<ObjectValue> proto = new ObjectValue;
        prototype = proto;
        prototype->put("constructor", object);
        object->put("prototype", proto);

    }
    ~ObjectConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Value* value = getScopeChain()->get("value");
        ObjectValue* object;
        if (value->isUndefined() || value->isNull())
        {
            object = new ObjectValue;
            object->setPrototype(prototype);
        }
        else
        {
            object = value->toObject();
        }
        return CompletionType(CompletionType::Return, object, "");
    }
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
        function->setParameterList(arguments);
        function->setScope(global);

        // Create Function.prototype
        prototype->setCode(new FunctionPrototype(prototype));
        prototype->put("constructor", function);

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

        report("param: '%s'\n", p.c_str());
        report("body: '%s'\n", body.c_str());

        setSource(p);
        FormalParameterList* formalParameterList = parseFormalParameterList();
        if (!formalParameterList)
        {
            throw getErrorInstance("SyntaxError");
        }

        setSource(body);
        yyparse();
        program->print();

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
    ObjectValue*            prototype;  // Error.prototype

public:
    ErrorConstructor(ObjectValue* error, const char* name, ObjectValue* function) :
        name(new StringValue(name)),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
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
        Register<ObjectValue> object = new ObjectValue;
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
        list->push(new StringValue(message));
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
            // program->print();

            program->process();

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

    // Register Object
    ObjectValue* object = new ObjectValue;
    object->setCode(new ObjectConstructor(object));
    global->put("Object", object);

    // Register Function
    ObjectValue* function = new ObjectValue;
    function->setCode(new FunctionConstructor(function));
    global->put("Function", function);

    object->setPrototype(function->getPrototype());
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

    // Register RegExp object
    ObjectValue* regexp = constructRegExpObject();
    global->put("RegExp", regexp);

    // Register Interface object
    global->put("Interface", constructInterfaceObject());

    // Register System object
    global->put("System", constructSystemObject(0));
}

//
// Execution Contexts
//

void setProgram(SourceElements* elements)
{
    program = elements;
}

ObjectValue* getGlobal()
{
    return global;
}

// 13.2.1
Value* ObjectValue::call(Value* self, ListValue* list)
{
    // Establish a new execution context using F's FormalParameterList,
    // the passed arguments list, and the this value as described in 10.2.3.
    ExecutionContext* context = new ExecutionContext(self, this, list);

    CompletionType result;
    Value*         value;
    try
    {
        result = code->evaluate();
        value = result.getValue();
    }
    catch (Value* e)
    {
        result.setType(CompletionType::Throw);
        value = e;
    }

    // Exit the execution context established in step 1, restoring the
    // previous execution context.
    delete context;

    if (result.isThrow())
    {
        throw value;
    }
    if (result.isReturn())
    {
        return value;
    }
    ASSERT(result.isNormal());
    return UndefinedValue::getInstance();
}

int main(int argc, char* argv[])
{
    // Check run-time system
    ASSERT(NAN != NAN);
    ASSERT(-0.0 == 0.0);

    srand48(0);

    if (argc < 2)
    {
        report("usage: %s filename\n", argv[0]);
        return EXIT_FAILURE;
    }

    report("%s %s\n", argv[0], argv[1]);

#ifdef __es__
    Handle<IContext> root = System()->getRoot();
    Handle<IFile> file = root->lookup(argv[1]);
    if (!file)
    {
        report("Could not open '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
    Handle<IStream> stream = file->getStream();
    if (!stream)
    {
        report("Could not open '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
    setSource(stream);
#else
    yyin = fopen(argv[1], "r");
    if (!yyin)
    {
        report("Could not open '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
#endif

    constructGlobalObject();
    ExecutionContext* context = new ExecutionContext(getGlobal(), getGlobal());
    Value::setThresh(1);

    report("alloc count: %lld\n", Value::getAllocCount());

    SourceElements* mainProgram;
    CompletionType result;
    try
    {
        yyparse();
        mainProgram = program;
#ifdef __es__
        stream = 0;
#else
        fclose(yyin);
#endif
        report("yyparse() ok.\n");

        mainProgram->print();

        mainProgram->process();

        result = mainProgram->evaluate();
    }
    catch (Value* value)
    {
        result.setType(CompletionType::Throw);
        result.setValue(value);
    }

    if (Register<Value> value = result.getValue())
    {
        value->print();
        report("\n");
    }
    result.setValue(0);

    report("alloc count: %lld\n", Value::getAllocCount());
    report("free count:  %lld\n", Value::getFreeCount());

    delete mainProgram;
    delete context;

    Value::sweep(true);
    ASSERT(Value::getAllocCount() - Value::getFreeCount() == 4);    // 4 for undefined, null, true, and false.

    report("done.\n");
    return EXIT_SUCCESS;
}

char* parseHex(const char* str, int limit, u32& hex)
{
    hex = 0;
    for (int i = 0; i < limit; ++i, ++str)
    {
        u8 x = *str;
        if (!isxdigit(x))
        {
            break;
        }
        if (isdigit(x))
        {
            x -= '0';
        }
        else
        {
            x = tolower(x);
            x -= 'a';
            x += 10;
        }
        hex <<= 4;
        hex |= x;
    }
    return (char*) str;
}

char* skipSpace(const char* str)
{
    static const char* rgsp[] =
    {
        // NBSP
        "\xc2\xa0",
        // LS
        "\xe2\x80\xa8",
        // PS
        "\xe2\x80\xa9",
        // USP
        "\xe1\x9a\x80",
        "\xe1\xa0\x8e",
        "\xe2\x80\x80",
        "\xe2\x80\x81",
        "\xe2\x80\x82",
        "\xe2\x80\x83",
        "\xe2\x80\x84",
        "\xe2\x80\x85",
        "\xe2\x80\x86",
        "\xe2\x80\x87",
        "\xe2\x80\x88",
        "\xe2\x80\x89",
        "\xe2\x80\x8a",
        "\xe2\x80\xaf",
        "\xe2\x81\x9f",
        "\xe3\x80\x80"
    };

    while (char c = *str)
    {
        if (strchr(" \t\v\f\n\r", c))
        {
            ++str;
        }
        else
        {
            int i;
            for (i = 0; i < sizeof(rgsp)/sizeof(rgsp[0]); ++i)
            {
                const char* sp = rgsp[i];
                size_t len = strlen(sp);
                if (strncmp(str, sp, len) == 0)
                {
                    str += len;
                    break;
                }
            }
            if (sizeof(rgsp)/sizeof(rgsp[0]) <= i)
            {
                return (char*) str;
            }
        }
    }
    return (char*) str;
}
