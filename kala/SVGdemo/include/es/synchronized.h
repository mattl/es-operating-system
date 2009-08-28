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

#ifndef NINTENDO_ES_SYNCHRONIZED_H_INCLUDED
#define NINTENDO_ES_SYNCHRONIZED_H_INCLUDED

/**
 * This template class provides a method to execute synchronized code
 * using a mutual exclusion lock.
 * @param I the class of the mutual exclusion lock.
 */
template<class I>
class Synchronized
{
    I& lock;

    Synchronized& operator=(const Synchronized&);

public:
    /**
     * Constructs a synchronized block using the specified mutual exclusion lock.
     * @param lock the mutual exclusion lock.
     */
    Synchronized(I& lock) : lock(lock)
    {
        lock->lock();
    }
    /**
     * Terminates this synchronized block.
     */
    ~Synchronized()
    {
        lock->unlock();
    }
};

#endif  // NINTENDO_ES_SYNCHRONIZED_H_INCLUDED
