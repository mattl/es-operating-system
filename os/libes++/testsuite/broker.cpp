/*
 * Copyright 2008, 2009 Google Inc.
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

#include <assert.h>
#include <stdio.h>
#include <es/any.h>
#include <es/broker.h>

class ITestInterface
{
public:
    virtual long rs32(long x) = 0;
    virtual float rf32(float x) = 0;
    virtual long long rs64(long long x) = 0;
    virtual double rf64(double x) = 0;
    virtual Any rv(Any x) = 0;
};

class TestImpl : public ITestInterface
{
public:
    long rs32(long x)
    {
        printf("%s(%ld)\n", __func__, x);
        return x;
    }

    float rf32(float x)
    {
        printf("%s(%g)\n", __func__, x);
        return x;
    }

    long long rs64(long long x)
    {
        printf("%s(%lld)\n", __func__, x);
        return x;
    }

    double rf64(double x)
    {
        printf("%s(%g)\n", __func__, x);
        return x;
    }

    Any rv(Any x)
    {
        printf("%s(%d)\n", __func__, static_cast<int32_t>(x));
        return x;
    }
};

TestImpl testObject;

long long invoke(void* self, void* base, int method, va_list ap)
{
    Any param[2];
    Any result;
    Any* variant;

    typedef void (**Vptr)(void* self, ...);
    Vptr vptr = *reinterpret_cast<Vptr*>(&testObject);

    switch (method) {
    case 0:
        param[0] = Any(reinterpret_cast<intptr_t>(self));
        param[1] = Any(va_arg(ap, int32_t));
        result = apply(2, param, reinterpret_cast<int32_t (*)()>(vptr[method]));
        break;
    case 1:
        param[0] = Any(reinterpret_cast<intptr_t>(self));
        param[1] = Any(va_arg(ap, int32_t));    // x86 only
        result = apply(2, param, reinterpret_cast<float (*)()>(vptr[method]));
        break;
    case 2:
        param[0] = Any(reinterpret_cast<intptr_t>(self));
        param[1] = Any(va_arg(ap, int64_t));
        result = apply(2, param, reinterpret_cast<int64_t (*)()>(vptr[method]));
        break;
    case 3:
        param[0] = Any(reinterpret_cast<intptr_t>(self));
        param[1] = Any(va_arg(ap, double));
        result = apply(2, param, reinterpret_cast<double (*)()>(vptr[method]));
        break;
    case 4:
        variant = static_cast<Any*>(self);
        self = va_arg(ap, void*);
        param[0] = Any(reinterpret_cast<intptr_t>(self));
        param[1] = Any(va_arg(ap, AnyBase));
        *variant = apply(2, param, reinterpret_cast<Any (*)()>(vptr[method]));
        result = Any(reinterpret_cast<intptr_t>(variant));
        break;
    }
    return evaluate(result);
}

Broker<invoke, 1> importer;

int main()
{
    ITestInterface* object = reinterpret_cast<ITestInterface*>(&(importer.getInterfaceTable())[0]);

    long rs32 = object->rs32(100);
    printf(": %d\n", rs32);
    assert(rs32 == 100);

    float rf32 = object->rf32(200.0f);
    printf(": %g\n", rf32);
    assert(rf32 == 200.0f);

    long long rs64 = object->rs64(300LL);
    printf(": %lld\n", rs64);
    assert(rs64 == 300LL);

    double rf64 = object->rf64(400.0);
    printf(": %g\n", rf64);
    assert(rf64 == 400.0);

    Any rv = object->rv(Any(500));
    printf(": %d\n", static_cast<int32_t>(rv));
    assert(static_cast<int32_t>(rv) == 500);

    printf("done.\n");
}
