/*
 * Copyright (c) 2006
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

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    int fd;

    if (argc == 3)
    {
        fd = truncate(argv[1], atoi(argv[2]));
        close(fd);
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
