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

#include <stdio.h>
#include <expat.h>

const char xml[] =
"<?xml version=\"1.0\"?>"
"<data>"
"<result month=\"Jul-2007\">"
"<apple>32</apple>"
"<orange>5</orange>"
"<banana>24</banana>"
"</result>"
"<result month=\"Aug-2007\">"
"<apple>12</apple>"
"<orange>43</orange>"
"<banana>8</banana>"
"</result>"
"</data>";

int depth;

void element_start(void *userData, const XML_Char *name, const XML_Char *atts[])
{
    for (int i = 0; i < depth; ++i)
    {
        printf("  ");
    }

    printf("%s ", name);
    for (int i = 0; atts[i]; i += 2)
    {
        printf(" %s='%s'", atts[i], atts[i + 1]);
    }
    printf("\n");
    ++depth;
}

void element_end(void *userData, const XML_Char *name)
{
    printf("\n");
    --depth;
}

void charhndl(void *userData, const XML_Char *s, int len)
{
    printf("'%*.*s'", len, len, s);
}

int main(int argc, char *argv[])
{
    int eofflag;
    size_t len;

    XML_Parser parser;
    if ((parser = XML_ParserCreate(NULL)) == NULL)
    {
        printf("parser creation error\n");
        return 1;
    }

    XML_SetElementHandler(parser, element_start, element_end);
    XML_SetCharacterDataHandler(parser, charhndl);

    if ((XML_Parse(parser, xml, sizeof xml - 1, 1)) == 0)
    {
        printf("parser error\n");
        return 1;
    }

    XML_ParserFree(parser);

    printf("done.\n");
    return 0;
}
