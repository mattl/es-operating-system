/*
 * Copyright 2008-2010 Google Inc.
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

extern es::CurrentProcess* System();

namespace
{
    SourceElements* program;
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

static es::Stream* lexStream;

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

void setSource(es::Stream* stream)
{
    lexStream = stream;
}

//
// Execution Contexts
//

SourceElements* getProgram()
{
    return program;
}

void setProgram(SourceElements* elements)
{
    program = elements;
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
        "\xe3\x80\x80",
        // BOM
        "\xef\xbb\xbf"
    };

    while (char c = *str)
    {
        if (strchr(" \t\v\f\n\r", c))
        {
            ++str;
            continue;
        }
        int i;
        for (i = 0; i < sizeof(rgsp) / sizeof(rgsp[0]); ++i)
        {
            const char* sp = rgsp[i];
            size_t len = strlen(sp);
            if (strncmp(str, sp, len) == 0)
            {
                str += len;
                break;
            }
        }
        if (sizeof(rgsp) / sizeof(rgsp[0]) <= i)
        {
            break;
        }
    }
    return const_cast<char*>(str);
}

int report(const char* spec, ...)
{
    va_list list;

    va_start(list, spec);
    es::Stream* output(System()->getOutput());
    Formatter formatter(output);
    formatter.setMode(Formatter::Mode::ECMAScript);
    int count = formatter.format(spec, list);
    output->release();
    va_end(list);
    return count;
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

    Handle<es::Context> root = System()->getRoot();
#ifdef __es__
    Handle<es::File> file = root->lookup(argv[1]);
    if (!file)
    {
        report("Could not open '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
    Handle<es::Stream> stream = file->getStream();
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
    Value::setThresh(1000);    // Set this thresh to one for testing GC

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

#ifdef VERBOSE
        mainProgram->print();
#endif
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
