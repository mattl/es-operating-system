/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

char buf[8*1024];

void usage(void)
{
    printf("usage: bm src dst [offset]\n");
}

int copy(int in, int out)
{
    long n;

    while (0 < (n = read(in, buf, sizeof(buf))))
    {
        if (write(out, buf, n) != n)
        {
            return -1;
        }
    }
    return (n == 0) ? 0 : -1;
}

int main(int argc, char* argv[])
{
    int in, out;
    long offset;

    if (argc < 3)
    {
        usage();
        return 1;
    }

    in = open(argv[1], O_BINARY | O_RDONLY);
    out = open(argv[2], O_BINARY | O_WRONLY);
    if (in < 0 || out < 0)
    {
        perror("bm");
        return 1;
    }

    if (4 <= argc)
    {
        offset = strtoul(argv[3], NULL, 0);
        if (lseek(out, offset, SEEK_SET) < 0)
        {
            perror("bm");
            return 1;
        }
    }

    if (copy(in, out) < 0)
    {
        perror("bm");
        return 1;
    }

    return 0;
}
