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

#include <es/dateTime.h>

const int DateTime::yearDays[13] =
{
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

const int DateTime::leapYearDays[13] =
{
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366
};
