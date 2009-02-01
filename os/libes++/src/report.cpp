/*
 * Copyright 2008, 2009 Google Inc.
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

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <es/dateTime.h>
#include <es/formatter.h>
#include <es/handle.h>
#include <es/timeSpan.h>
#include <es/ref.h>
#include <es/base/IProcess.h>

extern es::CurrentProcess* System();

extern "C"
{
    int esReport(const char* spec, ...) __attribute__((weak));
    int esReportv(const char* spec, va_list list) __attribute__((weak));
    void esPanic(const char* file, int line, const char* msg, ...) __attribute__((weak));
}

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
    es::Stream* output(System()->getOutput());
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

DateTime DateTime::getNow()
{
    return DateTime(System()->getNow());
}
