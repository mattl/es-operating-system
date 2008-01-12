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

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <es/base/IProcess.h>
#include <es/base/IThread.h>

using namespace es;

extern ICurrentProcess* System();

char buf[4096];

pthread_mutex_t mutex;

void* start(void* param)
{
    printf("start: %p\n", pthread_self());

    pthread_mutex_lock(&mutex);

    printf("start: sleep for three seconds.\n");
    sleep(3);

    pthread_mutex_unlock(&mutex);
    printf("start: done.\n");

    return 0;
}

int main(int argc, char* argv[])
{
    int fd;
    struct stat statbuf;

    printf("hello, world\n");

    fd = open("/file/cat.js", O_RDWR);
    printf("fd: %d\n", fd);
    if (fd == -1)
    {
        return 1;
    }
    int len = read(fd, buf, sizeof buf);
    if (len <= 0)
    {
        return 1;
    }

    write(1, buf, len);

    if (fstat(fd, &statbuf) == 0)
    {
        printf("%d's mode: %o\n", fd, statbuf.st_mode);
        printf("%d is a tty: %d\n", fd, isatty(fd));
    }

    close(fd);

    fd = 1;
    if (fstat(fd, &statbuf) == 0)
    {
        printf("%d's mode: %o\n", fd, statbuf.st_mode);
        printf("%d is a tty: %d\n", fd, isatty(fd));
    }

    DIR* dir = opendir("/file");
    if (dir)
    {
        struct dirent* ent;
        for (int i = 0; ent = readdir(dir); ++i)    // for getdents()
        {
            printf("%d: %s\n", i, ent->d_name);
        }
        closedir(dir);
    }

    for (int i = 0; i < 3; ++i)
    {
        sleep(1);   // for nanosleep()
        time_t t = time(0);
        printf("%s", ctime(&t));
    }

    // pthread test

    pthread_mutex_init(&mutex, 0);

    IThread* thread = System()->createThread((void*) start, 0);
    thread->start();

    sleep(1);
    printf("main: try to get lock.\n");
    pthread_mutex_lock(&mutex);
    printf("main: got lock.\n");
    pthread_mutex_unlock(&mutex);

    void* rval;
    thread->join(&rval);
    thread->release();
    printf("rval : %p : %p\n", pthread_self(), rval);

    pthread_mutex_destroy(&mutex);

    printf("done.\n");

    return 0;
}
