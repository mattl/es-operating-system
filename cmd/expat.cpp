/*
 * Copyright (c) 2007
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
