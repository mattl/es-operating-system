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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specification:
 *
 * Standard ECMA-262 ECMAScript Language Specification 3rd Edition, Dec 1999.
 * http://www.ecma-international.org/publications/standards/Ecma-262.htm
 *
 * ECMAScript Edition 3 Errata, June 9, 2003.
 * http://www.mozilla.org/js/language/E262-3-errata.html
 */

%{

extern int (*getLex())(char* buffer, int max_size);

#undef YY_INPUT
#define YY_INPUT(buffer, result, max_size)  \
{                                           \
    result = (getLex())(buffer, max_size);  \
}

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <set>
#include <stack>
#include "esjs.h"
#include "parser.h"

#ifndef VERBOSE
#define PRINTF(...)     (__VA_ARGS__)
#else
#define PRINTF(...)     report(__VA_ARGS__)
#endif

extern "C" int yyparse(void);

#undef YY_DECL
#define YY_DECL int yylex(YYSTYPE* yylval)

void yyerror(const char* str);

extern "C" int yywrap();

const char* enter_regex;

%}

/* regular definitions */

USP                     (\xe1\x9a\x80)|(\xe1\xa0\x8e)|(\xe2\x80[\x80-\x8a])|(\xe2\x80\xaf)|(\xe2\x81\x9f)|(\xe3\x80\x80)
LS                      (\xe2\x80\xa8)
PS                      (\xe2\x80\xa9)
NBSP                    (\xc2\xa0)
/* Lu up to \u00ff */
Lu                      (\xc3[\x80-\x9e])
/* Ll up to \u00ff */
Ll                      (\xc2[\xaa\xb5\xba])|(\xc3[\x9f-\xbf])
/* Unicode excluding USP, LS, PS, Lu and Li */
Unicode                 ([\xc4-\xdf][\x80-\xbf])|(\xe0[\xa0-\xbf][\x80-\xbf])|(\e1[\x80-\x99][\x80-\xbf])|(\e1\9a[\x81-\xbf])|(\e1[\x9b-\x9f][\x80-\xbf])|(\e1\xa0[\x80-\x8d\x8f-\xbf])|(\e1[\xa1-\xbf][\x80-\xbf])|(\e2\x80[\x8b-\xa7\xaa-\xbf])|(\e2\x81[\x80-\x9e\xa0-\xbf])|(\e2[\x82-\xbf][\x80-\xbf])|(\e3\x80[\x81-\xbf])|(\e3[\x81-\xbf][\x80-\xbf])|([\xe4-\xec][\x80-\xbf][\x80-\xbf])|(\xed[\x80-\x9f][\x80-\xbf])|([\xee-\xef][\x80-\xbf][\x80-\xbf])|(\xf0[\x90-\xbf][\x80-\xbf][\x80-\xbf])|([\xf1-\xf3][\x80-\xbf][\x80-\xbf][\x80-\xbf])|(\xf4[\x80-\x8f][\x80-\xbf][\x80-\xbf])

WhiteSpace              ([ \t\v\f]|{NBSP}|{USP})
LineTerminator          ([\n\r]|{LS}|{PS})
HexDigit                [0-9A-Fa-f]
DecimalDigit            [0-9]
HexIntegerLiteral       0(x|X){HexDigit}+
DecimalLiteral          ({DecimalIntegerLiteral}\.[0-9]*{ExponentPart}?)|(\.[0-9]+{ExponentPart}?)|({DecimalIntegerLiteral}{ExponentPart}?)

ExponentPart            (e|E)[\+\-]?[0-9]+
DecimalIntegerLiteral   0|([1-9][0-9]*)

SingleEscapeCharacter   ['\"\\bfnrtv]
NonEscapeCharacter      [^'\"\\bfnrtv\n\r]
HexEscapeSequence       x{HexDigit}{2}
UnicodeEscapeSequence   u{HexDigit}{4}
CharacterEscapeSequence {SingleEscapeCharacter}|{NonEscapeCharacter}
EscapeSequence          {CharacterEscapeSequence}|0|{HexEscapeSequence}|{UnicodeEscapeSequence}
SingleStringCharacter   ([^'\\\n\r]|\\{EscapeSequence})
DoubleStringCharacter   ([^\"\\\n\r]|\\{EscapeSequence})
IdentifierStart         ([A-Za-z$_]|{Lu}|{Ll}|{Unicode})
IdentifierPart          ([0-9]|{IdentifierStart}|(\\{UnicodeEscapeSequence}))
Identifier              {IdentifierStart}{IdentifierPart}*

MultiLineComment        \/\*(([^*])|(\*[^/]))*\*\/
SingleLineComment       \/\/

%x regex

RegularExpressionFirstChar  ([^\*\\\/\n\r]|\\[^\n\r])
RegularExpressionChars      ([^\\\/\n\r]|\\[^\n\r])
RegularExpressionBody       {RegularExpressionFirstChar}{RegularExpressionChars}*
RegularExpressionFlags      {IdentifierPart}*
RegularExpressionLiteral    \/{RegularExpressionBody}\/{RegularExpressionFlags}

%%

    if (enter_regex)
    {
        BEGIN(regex);
    }

{WhiteSpace}        { /* No action, and no return */ }
{LineTerminator}    { /* No action, and no return */ }

null                { return NULL_LITERAL; }
true                { return TRUE; }
false               { return FALSE; }
break               { return BREAK; }
case                { return CASE; }
catch               { return CATCH; }
continue            { return CONTINUE; }
default             { return DEFAULT; }
delete              { return DELETE; }
do                  { return DO; }
else                { return ELSE; }
finally             { return FINALLY; }
for                 { return FOR; }
function            { return FUNCTION; }
if                  { return IF; }
in                  { return IN; }
instanceof          { return INSTANCEOF; }
new                 { return NEW; }
return              { return RETURN; }
switch              { return SWITCH; }
this                { return THIS; }
throw               { return THROW; }
try                 { return TRY; }
typeof              { return TYPEOF; }
var                 { return VAR; }
void                { return VOID; }
while               { return WHILE; }
with                { return WITH; }
abstract            { return RESERVED; }
boolean             { return RESERVED; }
byte                { return RESERVED; }
char                { return RESERVED; }
class               { return RESERVED; }
const               { return RESERVED; }
debugger            { return RESERVED; }
double              { return RESERVED; }
enum                { return RESERVED; }
export              { return RESERVED; }
extends             { return RESERVED; }
final               { return RESERVED; }
float               { return RESERVED; }
goto                { return RESERVED; }
implements          { return RESERVED; }
import              { return RESERVED; }
int                 { return RESERVED; }
interface           { return RESERVED; }
long                { return RESERVED; }
native              { return RESERVED; }
package             { return RESERVED; }
private             { return RESERVED; }
protected           { return RESERVED; }
public              { return RESERVED; }
short               { return RESERVED; }
static              { return RESERVED; }
super               { return RESERVED; }
synchronized        { return RESERVED; }
throws              { return RESERVED; }
transient           { return RESERVED; }
volatile            { return RESERVED; }

"++"                { return OP_INC; }
--                  { return OP_DEC; }
==                  { return OP_EQ; }
!=                  { return OP_NE; }
===                 { return OP_SEQ; }
!==                 { return OP_SNE; }
"<"                 { return OP_LT; }
">"                 { return OP_GT; }
"<="                { return OP_LE; }
">="                { return OP_GE; }
"<<"                { return OP_SHL; }
">>"                { return OP_SHR; }
">>>"               { return OP_USHR; }
&&                  { return OP_AND; }
"||"                { return OP_OR; }
"*="                { return OP_MULA; }
"/="                { return OP_DIVA; }
"%="                { return OP_REMA; }
"+="                { return OP_PLUSA; }
"-="                { return OP_MINUSA; }
"<<="               { return OP_SHLA; }
">>="               { return OP_SHRA; }
">>>="              { return OP_USHRA; }
"&="                { return OP_ANDA; }
"^="                { return OP_XORA; }
"|="                { return OP_ORA; }

{Identifier}        {
                        PRINTF("%s\n", yytext);
                        yylval->identifier = new Identifier(yytext);
                        return IDENTIFIER;
                    }

{DecimalLiteral}    {
                        PRINTF("%s\n", yytext);
                        yylval->expression = new NumericLiteral(strtod(yytext, 0));
                        return NUMERIC_LITERAL;
                    }

{HexIntegerLiteral} {
                        PRINTF("%s\n", yytext);
                        yylval->expression = new NumericLiteral(strtoll(yytext, 0, 16));
                        return NUMERIC_LITERAL;
                    }

'{SingleStringCharacter}*'  {
                        PRINTF("%s\n", yytext);
                        yytext[yyleng - 1] = '\0';
                        yylval->expression = new StringLiteral(yytext + 1);
                        return STRING_LITERAL;
                    }

\"{DoubleStringCharacter}*\"    {
                        PRINTF("%s\n", yytext);
                        yytext[yyleng - 1] = '\0';
                        yylval->expression = new StringLiteral(yytext + 1);
                        return STRING_LITERAL;
                    }

{MultiLineComment}  {
                        /* MultiLineComment */
                    }

{SingleLineComment} {
                        /* SingleLineComment */
                        int c;

                        do
                        {
                            c = yyinput();
                        } while (c != '\n' && c != '\r' && c != EOF);
                    }

<regex>{RegularExpressionBody}\/{RegularExpressionFlags} {
                        yylval->expression = new RegularExpressionLiteral(std::string(enter_regex) + yytext);
                        enter_regex = 0;
                        BEGIN(INITIAL);
                        return REGULAR_EXPRESSION_LITERAL;
                    }

.                   { return (int) yytext[0]; }

%%
