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

/*
 * These coded instructions, statements, and computer programs use
 * the following software:
 *
 * PCRE (Perl-compatible regular expression library)
 *
 * Copyright (c) 1997-2007 University of Cambridge
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of the University of Cambridge nor the name of Google
 *       Inc. nor the names of their contributors may be used to endorse or
 *       promote products derived from this software without specific prior
 *       written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The latest release of PCRE is always available from
 *
 *   ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-xxx.tar.gz
 */

#include "esjs.h"
#include "parser.h"
#include "interface.h"
#include <pcreposix.h>

//
// RegExp Object Value
//

class RegExpValue : public ObjectValue
{
    static ObjectValue* prototype;  // RegExp.prototype

    bool    global;
    regex_t regex;

public:
    RegExpValue(const char* pattern, int cflags, bool global) :
        global(global)
    {
        int code = regcomp(&regex, pattern, REG_UTF8 | cflags);
        if (code)
        {
            throw getErrorInstance("SyntaxError");
        }

        setPrototype(prototype);
    }

    ~RegExpValue()
    {
        regfree(&regex);
    }

    const char* getClass() const
    {
        return "RegExp";
    }

    int match(std::string& s, int nmatch, regmatch_t* pmatch, int offset = 0)
    {
        if (offset < 0 || s.length() < offset)
        {
            return REG_NOMATCH;
        }

        int code = regexec(&regex, s.c_str() + offset, nmatch, pmatch,
                           (offset == 0) ? 0 : REG_NOTBOL);
        if (code == 0)
        {
            int index = 0;
            for (int i = 0; i < nmatch; ++i)
            {
                if (index <= pmatch[i].rm_so)    // Check wrap around.
                {
                    index = pmatch[i].rm_so;
                    pmatch[i].rm_so += offset;
                    pmatch[i].rm_eo += offset;
                }
                else
                {
                    pmatch[i].rm_so = -1;
                    pmatch[i].rm_eo = -1;
                }
            }
        }
        return code;
    }

    Value* exec(std::string s)
    {
        int offset = global ? (int) get("lastIndex")->toNumber() : 0;
        regmatch_t pmatch[getCapturingParens() + 1];
        int code = match(s, getCapturingParens() + 1, pmatch, offset);
        if (code /* == REG_NOMATCH */)
        {
            return NullValue::getInstance();
        }
        else
        {
            if (global)
            {
                Register<NumberValue> lastIndex = new NumberValue(pmatch[0].rm_eo);
                put("lastIndex", lastIndex);
            }

            Register<ArrayValue> array = new ArrayValue;
            Register<NumberValue> start = new NumberValue(pmatch[0].rm_so);
            array->put("index", start);
            Register<StringValue> input = new StringValue(s);
            array->put("input", input);
            for (unsigned i = 0; i <= regex.re_nsub; ++i)
            {
                char name[12];
                sprintf(name, "%d", i);
                if (0 <= pmatch[i].rm_so)    // Check wrap around.
                {
                    Register<StringValue> substring =
                        new StringValue(s.substr(pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so));
                    array->put(name, substring);
                }
                else
                {
                    array->put(name, UndefinedValue::getInstance());
                }
            }
            return array;
        }
    }

    int getCapturingParens()
    {
        return regex.re_nsub;
    }

    bool isGlobal()
    {
        return global;
    }

    friend class RegExpConstructor;
    friend class RegExpMethod;
};

ObjectValue* RegExpValue::prototype;

//
// RegExp Methods
//

class RegExpMethod : public Code
{
    enum Method
    {
        Exec,
        Test,
        ToString,
        MethodCount
    };

    FormalParameterList*    arguments;
    enum Method             method;

    static const char*      names[MethodCount];

    Value* exec(RegExpValue* regexp)
    {
        return regexp->exec(getScopeChain()->get("string")->toString());
    }

    Value* test(RegExpValue* regexp)
    {
        Register<Value> value = exec(regexp);
        return BoolValue::getInstance(!value->isNull());
    }

    Value* toString(RegExpValue* regexp)
    {
        std::string s("/");
        s += regexp->get("source")->toString();
        s += "/";
        if (regexp->global)
        {
            s += "g";
        }
        if (regexp->get("ignoreCase")->toBoolean())
        {
            s += "i";
        }
        if (regexp->get("multiline")->toBoolean())
        {
            s += "m";
        }
        return new StringValue(s);
    }

public:
    RegExpMethod(ObjectValue* function, int method) :
        arguments(new FormalParameterList),
        method((enum Method) method)
    {
        ASSERT(0 <= method);
        ASSERT(method < MethodCount);
        switch (method)
        {
        case Exec:
        case Test:
            arguments->add(new Identifier("string"));
            break;
        default:
            break;
        }

        function->setParameterList(arguments);
        function->setScope(getGlobal());
    }
    ~RegExpMethod()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        RegExpValue* regexp = dynamic_cast<RegExpValue*>(getThis());
        if (!regexp)
        {
            throw getErrorInstance("TypeError");
        }

        Register<Value> value;
        switch (method)
        {
        case Exec:
            value = exec(regexp);
            break;
        case Test:
            value = test(regexp);
            break;
        case ToString:
            value = toString(regexp);
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

const char* RegExpMethod::names[] =
{
    "exec",
    "test",
    "toString"
};

//
// RegExp Constructor
//

class RegExpConstructor : public Code
{
    FormalParameterList*    arguments;
    ObjectValue*            prototype;  // String.prototype

public:
    RegExpConstructor(ObjectValue* regexp) :
        arguments(new FormalParameterList),
        prototype(new ObjectValue)
    {
        ObjectValue* function = static_cast<ObjectValue*>(getGlobal()->get("Function"));

        arguments->add(new Identifier("pattern"));
        arguments->add(new Identifier("flags"));

        prototype->put("constructor", regexp);
        prototype->setPrototype(function->getPrototype()->getPrototype());

        for (int i = 0; i < RegExpMethod::methodCount(); ++i)
        {
            ObjectValue* function = new ObjectValue;
            RegExpMethod* method = new RegExpMethod(function, i);
            function->setCode(method);
            prototype->put(method->name(), function);
        }

        regexp->setParameterList(arguments);
        regexp->setScope(getGlobal());
        regexp->put("prototype", prototype);
        regexp->setPrototype(function->getPrototype());

        RegExpValue::prototype = prototype;
    }
    ~RegExpConstructor()
    {
        delete arguments;
    }

    FormalParameterList* getFormalParameterList()
    {
        return arguments;
    }

    CompletionType evaluate()
    {
        Register<Value> pattern = getScopeChain()->get("pattern");
        Register<Value> flags = getScopeChain()->get("flags");
        return CompletionType(CompletionType::Return, construct(pattern, flags), "");
    }

    static RegExpValue* construct(Value* pattern, Value* flags)
    {
        if (dynamic_cast<RegExpValue*>(pattern))
        {
            if (!flags->isUndefined())
            {
                throw getErrorInstance("TypeError");
            }
            return static_cast<RegExpValue*>(pattern);
        }

        bool global(false);
        int cflags = 0;
        if (!flags->isUndefined())
        {
            const char* gim = flags->toString().c_str();
            while (char c = *gim++)
            {
                switch (c)
                {
                case 'g':
                    if (global)
                    {
                        throw getErrorInstance("SyntaxError");
                    }
                    global = true;
                    break;
                case 'i':
                    if (cflags & REG_ICASE)
                    {
                        throw getErrorInstance("SyntaxError");
                    }
                    cflags |= REG_ICASE;
                    break;
                case 'm':
                    if (cflags & REG_NEWLINE)
                    {
                        throw getErrorInstance("SyntaxError");
                    }
                    cflags |= REG_NEWLINE;
                    break;
                default:
                    throw getErrorInstance("SyntaxError");
                    break;
                }
            }
        }

        const char* regex;
        if (pattern->isUndefined())
        {
            regex = "";
        }
        else
        {
            regex = pattern->toString().c_str();
        }
        Register<RegExpValue> object = new RegExpValue(regex, cflags, global);

        object->put("global", BoolValue::getInstance(global));
        object->put("ignoreCase", BoolValue::getInstance(cflags & REG_ICASE));
        object->put("multiline", BoolValue::getInstance(cflags & REG_NEWLINE));

        object->put("source", pattern);

        Register<NumberValue> lastIndex = new NumberValue(0);
        object->put("lastIndex", lastIndex);

        return object;
    }
};

ObjectValue* constructRegExpObject()
{
    ObjectValue* regexp = new ObjectValue;
    regexp->setCode(new RegExpConstructor(regexp));
    return regexp;
}

//
// String methods related to RegExp objects
//

Value* stringMatch()
{
    Value* param = getScopeChain()->get("regexp");
    Register<RegExpValue> regexp = dynamic_cast<RegExpValue*>(param);
    if (!static_cast<RegExpValue*>(regexp))
    {
        regexp = RegExpConstructor::construct(param, UndefinedValue::getInstance());
    }
    std::string s = getThis()->toString();

    if (!regexp->isGlobal())
    {
        return regexp->exec(s);
    }

    Register<NumberValue> lastIndex = new NumberValue(0);
    regexp->put("lastIndex", lastIndex);

    Register<ArrayValue> array = new ArrayValue;
    int offset = (int) regexp->get("lastIndex")->toNumber();
    for (int i = 0; ; ++i)
    {
        regmatch_t pmatch[regexp->getCapturingParens() + 1];
        int code = regexp->match(s, regexp->getCapturingParens() + 1, pmatch, offset);
        if (code /* == REG_NOMATCH */)
        {
            break;
        }

        offset = pmatch[0].rm_eo;
        lastIndex = new NumberValue(offset);
        regexp->put("lastIndex", lastIndex);

        char name[12];
        sprintf(name, "%d", i);
        Register<StringValue> substring = new StringValue(s.substr(pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so));
        array->put(name, substring);
    }
    return array;
}

Value* stringReplace()
{
    std::string s = getThis()->toString();
    std::string t = s;
    int diff = 0;

    Value* searchValue = getScopeChain()->get("searchValue");
    RegExpValue* regexp = dynamic_cast<RegExpValue*>(searchValue);

    int nmatch = 1;
    if (regexp)
    {
        nmatch += regexp->getCapturingParens();
        if (regexp->isGlobal())
        {
            Register<NumberValue> lastIndex = new NumberValue(0);
            regexp->put("lastIndex", lastIndex);
        }
    }
    regmatch_t pmatch[nmatch];

    int offset = 0;
    do
    {
        if (regexp)
        {
            int code = regexp->match(s, nmatch, pmatch, offset);
            if (code /* == REG_NOMATCH */)
            {
                break;
            }

            if (regexp->isGlobal())
            {
                offset = pmatch[0].rm_eo;
                Register<NumberValue> lastIndex = new NumberValue(offset);
                regexp->put("lastIndex", lastIndex);
            }
        }
        else
        {
            unsigned pos = s.find(searchValue->toString());
            if (pos == std::string::npos)
            {
                break;
            }

            pmatch[0].rm_so = pos;
            pmatch[0].rm_eo = pos + searchValue->toString().length();
        }

        Value* replaceValue = getScopeChain()->get("replaceValue");
        ObjectValue* function = dynamic_cast<ObjectValue*>(replaceValue);
        std::string n;
        if (function && function->getCode())
        {
            Register<ListValue> list = new ListValue;
            Register<Value> value;
            for (int i = 0; i < nmatch; ++i)
            {
                value = new StringValue(s.substr(pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so));
                list->push(value);
            }
            value = new NumberValue(pmatch[0].rm_so);
            list->push(value);
            value = new StringValue(s);
            list->push(value);
            value = function->call(NullValue::getInstance(), list);
            n = value->toString();
        }
        else
        {
            std::string r = replaceValue->toString();
            for (const char* p = r.c_str(); char c = *p; ++p)
            {
                if (c != '$')
                {
                    n += c;
                    continue;
                }

                c = *++p;
                switch (c)
                {
                case '$':
                    n += '$';
                    break;
                case '&':
                    n += s.substr(pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so);
                    break;
                case '`':
                    n += s.substr(0, pmatch[0].rm_so - 0);
                    break;
                case '\'':
                    n += s.substr(pmatch[0].rm_eo, s.length() - pmatch[0].rm_eo);
                    break;
                case '\0':
                    n += '$';
                    --p;
                    break;
                default:
                    if (isdigit(c))
                    {
                        int sub = c - '0';
                        c = *++p;
                        if (isdigit(c))
                        {
                            sub *= 10;
                            sub += c - '0';
                        }
                        else
                        {
                            --p;
                        }
                        if (0 < sub && sub < nmatch && 0 <= pmatch[sub].rm_so)
                        {
                            n += s.substr(pmatch[sub].rm_so, pmatch[sub].rm_eo - pmatch[sub].rm_so);
                        }
                    }
                    else
                    {
                        n += '$';
                        n += c;
                    }
                    break;
                }
            }

        }

        t.replace(pmatch[0].rm_so + diff, pmatch[0].rm_eo - pmatch[0].rm_so, n);
        diff += n.length() - (pmatch[0].rm_eo - pmatch[0].rm_so);

    } while (regexp && regexp->isGlobal());

    return new StringValue(t);
}

Value* stringSearch()
{
    Value* param = getScopeChain()->get("regexp");
    Register<RegExpValue> regexp = dynamic_cast<RegExpValue*>(param);
    if (!static_cast<RegExpValue*>(regexp))
    {
        regexp = RegExpConstructor::construct(param, UndefinedValue::getInstance());
    }
    std::string s = getThis()->toString();

    regmatch_t pmatch[regexp->getCapturingParens() + 1];
    int code = regexp->match(s, regexp->getCapturingParens() + 1, pmatch);
    return new NumberValue(code /* == REG_NOMATCH */ ? -1 : pmatch[0].rm_so);
}

static int splitMatch(Value* reg, std::string& s, int q, int nmatch, regmatch_t* pmatch)
{
    RegExpValue* regexp = dynamic_cast<RegExpValue*>(reg);
    if (!regexp)
    {
        std::string r = reg->toString();
        unsigned pos = s.substr(q).find(r);
        if (pos == std::string::npos)
        {
            return REG_NOMATCH;
        }
        pos += q;
        pmatch[0].rm_so = pos;
        pmatch[0].rm_eo = pos + r.length();
        return 0;
    }
    else
    {
        return regexp->match(s, nmatch, pmatch, q);
    }
}

Value* stringSplit()
{
    std::string s = getThis()->toString();

    Register<ArrayValue> a = new ArrayValue;

    Value* limit = getScopeChain()->get("limit");
    u32 lim = limit->isUndefined() ? 4294967295u : limit->toUint32();

    unsigned p = 0;

    Value* separator = getScopeChain()->get("separator");
    Register<Value> r;
    RegExpValue* regexp = dynamic_cast<RegExpValue*>(separator);
    int nmatch;
    if (regexp)
    {
        r = separator;
        nmatch = regexp->getCapturingParens() + 1;
    }
    else
    {
        r = new StringValue(separator->toString());
        nmatch = 1;
    }
    regmatch_t pmatch[nmatch];

    if (lim == 0)
    {
        return a;
    }

    u32 length = 0; // a.length
    if (!separator->isUndefined())
    {
        if (0 < s.length())
        {
            char name[12];
            unsigned  q = p;

            while (q < s.length() && splitMatch(r, s, q, nmatch, pmatch) == 0)
            {
                unsigned e = pmatch[0].rm_eo;                    // endIndex
                if (e == p)                                 // 15
                {
                    ASSERT(pmatch[0].rm_so == q);
                    u32 utf32;
                    const char* u = s.c_str();
                    q = utf8to32(u + q, &utf32) - u;        // move q to the next character
                    continue;
                }

                ASSERT(p <= pmatch[0].rm_so);
                q = pmatch[0].rm_so;
                std::string t = s.substr(p, q - p);         // 16
                Register<StringValue> stringValue = new StringValue(t);
                sprintf(name, "%u", length);
                a->put(name, stringValue);                  // 17
                if (lim == ++length)
                {
                    return a;                               // 18
                }

                p = e;
                for (int i = 1; i < nmatch; ++i)
                {
                    Register<Value> value;
                    if (0 <= pmatch[i].rm_so)
                    {
                        std::string t = s.substr(pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so);
                        value = new StringValue(t);
                    }
                    else
                    {
                        value = UndefinedValue::getInstance();
                    }
                    sprintf(name, "%u", length);
                    a->put(name, value);                    // 23
                    if (lim == ++length)
                    {
                        return a;                           // 24
                    }
                }
                q = p;
            }

            ASSERT(p <= s.length());
            std::string t = s.substr(p, s.length() - p);    // 28
            Register<StringValue> stringValue = new StringValue(t);
            sprintf(name, "%u", length);
            a->put(name, stringValue);                      // 29
            return a;                                       // 30
        }
        else if (splitMatch(r, s, 0, nmatch, pmatch) == 0)  // 31
        {
            return a;
        }
    }

    Register<StringValue> stringValue = new StringValue(s);
    a->put("0", stringValue);                               // 33
    return a;
}

//
// RegularExpressionLiteral
//

Value* RegularExpressionLiteral::evaluate()
{
    // XXX 'construct' should be done just once.
    int slash = value.rfind('/');
    Register<Value> pattern = new StringValue(value.substr(1, slash - 1));
    Register<Value> flags = new StringValue(value.substr(slash + 1));
    return RegExpConstructor::construct(pattern, flags);
}
