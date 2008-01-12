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
