/*
 * Copyright 2008, 2009 Google Inc.
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

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <es/object.h>
#include <es/base/IProcess.h>
#include <es/base/IThread.h>

extern es::CurrentProcess* System();

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

    es::Thread* thread = System()->createThread((void*) start, 0);
    thread->start();

    sleep(1);
    printf("main: try to get lock.\n");
    pthread_mutex_lock(&mutex);
    printf("main: got lock.\n");
    pthread_mutex_unlock(&mutex);

    void* rval = thread->join();
    thread->release();
    printf("rval : %p : %p\n", pthread_self(), rval);

    pthread_mutex_destroy(&mutex);

    printf("done.\n");

    return 0;
}
