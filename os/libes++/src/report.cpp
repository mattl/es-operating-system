/*
 * Copyright (c) 2006, 2007
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

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <es/dateTime.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/timeSpan.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include <es/base/IProcess.h>

using namespace es;

extern ICurrentProcess* System();

extern "C"
{
    int esReport(const char* spec, ...) __attribute__((weak));
    int esReportv(const char* spec, va_list list) __attribute__((weak));
    void esPanic(const char* file, int line, const char* msg, ...) __attribute__((weak));
}

void* esCreateInstance(const Guid& rclsid, const Guid& riid) __attribute__((weak));

int esReport(const char* spec, ...)
{
    va_list list;
    int count;

    va_start(list, spec);
    count = esReportv(spec, list);
    va_end(list);
    return count;
}

int esReportv(const char* spec, va_list list)
{
    IStream* output(System()->getOutput());
    Formatter textOutput(output);
    int count = textOutput.format(spec, list);
    output->release();
    return count;
}

void esPanic(const char* file, int line, const char* msg, ...)
{
    va_list marker;

    va_start(marker, msg);
    esReportv(msg, marker);
    va_end(marker);
    esReport(" in \"%s\" on line %d.\n", file, line);

    System()->exit(1);
}

void* esCreateInstance(const Guid& rclsid, const Guid& riid)
{
    static Handle<IClassStore> classStore;

    if (!classStore)
    {
        Handle<IContext> root = System()->getRoot();
        classStore = root->lookup("class");
    }
    return classStore->createInstance(rclsid, riid);
}

DateTime DateTime::getNow()
{
    return DateTime(System()->getNow());
}
