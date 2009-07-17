/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#include <math.h>
#include <es/utf.h>
#include "esjs.h"
#include "parser.h"
#include "interface.h"

extern Value* stringMatch();
extern Value* stringReplace();
extern Value* stringSearch();
extern Value* stringSplit();

//
// String Methods
//

class StringMethod : public Code
{
    enum Method
    {
        ToString,
        ValueOf,
        CharAt,
        CharCodeAt,
        Concat,
        IndexOf,
        LastIndexOf,
        LocaleCompare,
        Match,
        Replace,
        Search,
        Slice,
        Split,
        Substring,
        Substr,
        ToLowerCase,
        ToLocaleLowerCase,
        ToUpperCase,
        ToLocaleUpperCase,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    // Note charAt(pos) for esjs returns a string which contains a byte
    // sequence of a valid UTF-8 character.
    // i.e., The length of the returned string can be greater than one.
    Value* charAt()
    {
        std::string s = getThis()->toString();
        double pos = getScopeChain()->get("pos")->toInteger();
        if (0.0 <= pos && pos < s.length())
        {
            u32 utf32;
            int offset = static_cast<int>(pos);
            char* next = utf8to32(s.c_str() + offset, &utf32);
            if (next)
            {
                return new StringValue(s.substr(offset, next - (s.c_str() + offset)));
            }
        }
        return new StringValue("");
    }

    Value* charCodeAt()
    {
        std::string s = getThis()->toString();
        double pos = getScopeChain()->get("pos")->toInteger(); // pos represents a number of bytes.
        if (0.0 <= pos && pos < s.length())
        {
            u32 utf32;
            int offset = static_cast<int>(pos);
            char* next = utf8to32(s.c_str() + offset, &utf32);
            if (next)
            {
                return new NumberValue(utf32);
            }
        }
        return new NumberValue(NAN);
    }

    Value* concat(Value* value)
    {
        std::string result;
        result = value->toString();

        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));
        u32 size = list->length();
        for (u32 i = 0; i < size; ++i)
        {
            std::string code = (*list)[i]->toString();
            result += code;
        }

        return new StringValue(result);
    }

    Value* indexOf(Value* value)
    {
        Value* position = getScopeChain()->get("position");
        Value* searchString = getScopeChain()->get("searchString");

        std::string s = value->toString();
        std::string pattern = searchString->toString();

        double pos;
        if (position->isUndefined())
        {
            pos = 0.0;
        }
        else
        {
            pos = position->toInteger();
        }
        int len = s.length();
        pos = std::min(std::max(pos, 0.0), static_cast<double>(len));
        pos = s.find(pattern, static_cast<int>(pos));
        if (pos == std::string::npos)
        {
            pos = -1.0;
        }

        return new NumberValue(pos);
    }

    Value* lastIndexOf(Value* value)
    {
        Value* position = getScopeChain()->get("position");
        Value* searchString = getScopeChain()->get("searchString");

        std::string s = value->toString();
        std::string pattern = searchString->toString();

        double pos = position->toNumber();
        int len = s.length();
        pos = std::min(std::max(pos, 0.0), static_cast<double>(len));
        pos = s.rfind(pattern, static_cast<int>(pos));
        if (pos == std::string::npos)
        {
            pos = -1.0;
        }

        return new NumberValue(pos);
    }

    Value* localeCompare(Value* value)
    {
        std::string s = value->toString();
        std::string that= getScopeChain()->get("that")->toString();
        return new NumberValue(s.compare(that));
    }

    Value* slice(Value* value)
    {
        std::string s = value->toString();
        double len = s.length();
        double start = getScopeChain()->get("start")->toInteger();
        Value* endValue = getScopeChain()->get("end");
        double end = endValue->isUndefined() ? len : endValue->toInteger();
        if (start < 0)
        {
            start = std::max(start + len, 0.0);
        }
        else
        {
            start = std::min(start, len);
        }
        if (end < 0)
        {
            end = std::max(end + len, 0.0);
        }
        else
        {
            end = std::min(end, len);
        }
        len = std::max(end - start, 0.0);
        s = s.substr((int) start, (int) len);
        return new StringValue(s);
    }

    Value* substring(Value* value)
    {
        std::string s = value->toString();
        double len = s.length();
        double start = getScopeChain()->get("start")->toInteger();
        Value* endValue = getScopeChain()->get("end");
        double end = endValue->isUndefined() ? len : endValue->toInteger();
        start = std::min(std::max(start, 0.0), len);
        end = std::min(std::max(end, 0.0), len);
        if (end < start)
        {
            std::swap(start, end);
        }
        s = s.substr((int) start, (int) (end - start));
        return new StringValue(s);
    }

    Value* substr(Value* value)
    {
        std::string s = value->toString();
        double start = getScopeChain()->get("start")->toInteger();

        Value* lengthValue = getScopeChain()->get("length");
        double length;
        if (lengthValue->isUndefined())
        {
            length = INFINITY;
        }
        else
        {
            length = lengthValue->toInteger();
        }

        int len = s.length();

        if (start < 0.0)
        {
            start = std::max(start + len, 0.0);
        }

        length = std::min(std::max(length, 0.0), len - start);
        if (length < 0.0)
        {
            return new StringValue("");
        }
        s = s.substr(static_cast<int>(start), static_cast<int>(length));
        return new StringValue(s);
    }

    Value* toLowerCase(Value* value)
    {
        std::string result;
        const char* next = value->toString().c_str();
        while (next && *next)
        {
            char utf8[5];
            u32 utf32;

            next = utf8to32(next, &utf32);
            char* nextResult = utf32to8(utftolower(utf32), utf8);
            if (nextResult)
            {
                *nextResult = '\0';
                result += utf8;
            }
        }
        return new StringValue(result);
    }

    Value* toUpperCase(Value* value)
    {
        std::string result;
        const char* next = value->toString().c_str();
        while (next && *next)
        {
            char utf8[5];
            u32 utf32;

            next = utf8to32(next, &utf32);
            char* nextResult = utf32to8(utftoupper(utf32), utf8);
            if (nextResult)
            {
                *nextResult = '\0';
                result += utf8;
            }
        }
        return new StringValue(result);
    }

public:
    StringMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case CharAt:
        case CharCodeAt:
            arguments->add(new Identifier("pos"));
            break;
        case IndexOf:
        case LastIndexOf:
            arguments->add(new Identifier("searchString"));
            arguments->add(new Identifier("position"));
            break;
        case LocaleCompare:
            arguments->add(new Identifier("that"));
            break;
        case Match:
        case Search:
            arguments->add(new Identifier("regexp"));
            break;
        case Replace:
            arguments->add(new Identifier("searchValue"));
            arguments->add(new Identifier("replaceValue"));
            break;
        case Slice:
        case Substring:
            arguments->add(new Identifier("start"));
            arguments->add(new Identifier("end"));
            break;
        case Substr:
            arguments->add(new Identifier("start"));
            arguments->add(new Identifier("length"));
            break;
        case Split:
            arguments->add(new Identifier("separator"));
            arguments->add(new Identifier("limit"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~StringMethod()
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
        switch (method)
        {
        case ToString:
        case ValueOf:
            if (!getThis()->isObject())
            {
                throw getErrorInstance("TypeError");
            }
            value = static_cast<ObjectValue*>(getThis())->getValueProperty();
            if (!value->isString())
            {
                throw getErrorInstance("TypeError");
            }
            break;
        case CharAt:
            value = charAt();
            break;
        case CharCodeAt:
            value = charCodeAt();
            break;
        case Concat:
            value = concat(getThis());
            break;
        case IndexOf:
            value = indexOf(getThis());
            break;
        case LastIndexOf:
            value = lastIndexOf(getThis());
            break;
        case LocaleCompare:
            value = localeCompare(getThis());
            break;
        case Match:
            value = stringMatch();
            break;
        case Replace:
            value = stringReplace();
            break;
        case Search:
            value = stringSearch();
            break;
        case Slice:
            value = slice(getThis());
            break;
        case Split:
            value = stringSplit();
            break;
        case Substring:
            value = substring(getThis());
            break;
        case Substr:
            value = substr(getThis());
            break;
        case ToLowerCase:
        case ToLocaleLowerCase:
            value = toLowerCase(getThis());
            break;
        case ToUpperCase:
        case ToLocaleUpperCase:
            value = toUpperCase(getThis());
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

const char* StringMethod::names[] =
{
    "toString",
    "valueOf",
    "charAt",
    "charCodeAt",
    "concat",
    "indexOf",
    "lastIndexOf",
    "localeCompare",
    "match",
    "replace",
    "search",
    "slice",
    "split",
    "substring",
    "substr",
    "toLowerCase",
    "toLocaleLowerCase",
    "toUpperCase",
    "toLocaleUpperCase",
};

//
// StringConstructor Methods
//

class StringConstructorMethod : public Code
{
    enum Method
    {
        FromCharCode,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* fromCharCode()
    {
        ListValue* list = static_cast<ListValue*>(getScopeChain()->get("arguments"));

        u32 numChars = list->length();

        std::string result = "";
        u16 utf16;
        for (u32 i = 0; i < numChars; ++i)
        {
            utf16 = static_cast<u16>((*list)[i]->toUint16());
            char utf8[5];
            char* next = utf32to8(utf16, utf8);
            if (next)
            {
                *next = 0;
                result += utf8;
            }
        }
        return new StringValue(result);
    }

public:
    StringConstructorMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~StringConstructorMethod()
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
        switch (method)
        {
        case FromCharCode:
            value = fromCharCode();
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

const char* StringConstructorMethod::names[] =
{
    "fromCharCode",
};

//
// String Constructor
//

class StringConstructor : public Code
{
    ObjectValue*            string;
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // String.prototype

public:
    StringConstructor(ObjectValue* string) :
        string(string),
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("value"));
        prototype->put("constructor", string);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < StringMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            StringMethod* method = new StringMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        string->setParameterList(arguments);
        string->setScope(getGlobal());
        string->put("prototype", prototype);
        string->setPrototype(function->getPrototype());

        for (int i = 0; i < StringConstructorMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            StringConstructorMethod* method = new StringConstructorMethod(function, i);
            function->setCode(method);
            string->put(method->name(), function);
        }
    }
    ~StringConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<Value> value = getScopeChain()->get("value");
        Register<StringValue> result = new StringValue(value->isUndefined() ? "" : value->toString());

        if (string->hasInstance(getThis()))
        {
            // Constructor
            ObjectValue* object = static_cast<ObjectValue*>(getThis());
            object->setValueProperty(result);
            Register<NumberValue> length = new NumberValue(result->length());
            object->put("length", length,
                        ObjectValue::DontEnum | ObjectValue::DontDelete | ObjectValue::ReadOnly);
        }

        return CompletionType(CompletionType::Return, result, "");
    }
};

ObjectValue*
StringValue::toObject()
{
    // Create a new String object
    ObjectValue* string = dynamic_cast<ObjectValue*>(getGlobal()->get("String"));
    if (!string || !string->getCode())
    {
        throw getErrorInstance("TypeError");
    }
    Register<ListValue> list = new ListValue;
    list->push(this);
    return string->construct(list);
};

ObjectValue* constructStringObject()
{
    ObjectValue* string = new ObjectValue;
    string->setCode(new StringConstructor(string));
    return string;
}
