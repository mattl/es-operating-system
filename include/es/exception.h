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

#ifndef NINTENDO_ES_EXCEPTION_H_INCLUDED
#define NINTENDO_ES_EXCEPTION_H_INCLUDED

/**
 * This represents an exception.
 */
class Exception
{
public:
    /**
     * Gets the result of this exception.
     */
    virtual int getResult() const = 0;
};

/**
 * This template class represents a system exception with an error number.
 * @param result the error number to be returned.
 */
template <int result>
class SystemException : public Exception
{
public:
    /**
     * Gets the error number of this system exception.
     */
    virtual int getResult() const
    {
        return result;
    }
};

#endif  // #ifndef NINTENDO_ES_EXCEPTION_H_INCLUDED
