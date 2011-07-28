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

#ifndef NINTENDO_ES_TIMER_H_INCLUDED
#define NINTENDO_ES_TIMER_H_INCLUDED

#include <new>
#include <es/dateTime.h>
#include <es/synchronized.h>
#include <es/timeSpan.h>
#include <es/tree.h>
#include <es/base/IMonitor.h>
#include <es/base/IThread.h>

es::Thread* esCreateThread(void* (*start)(void* param), void* param);

class Timer;

class TimerTask
{
    friend class Timer;

    DateTime    executionTime;
    TimeSpan    period;

public:
    TimerTask() : executionTime(0), period(0)
    {
    }

    DateTime scheduledExecutionTime()
    {
        return executionTime;
    }

    bool isEnabled()
    {
        return (executionTime != 0) ? true : false;
    }

    bool isPeriodic()
    {
        return (period != 0) ? true : false;
    }

    virtual void run() = 0;
};

class Timer
{
    friend class TimerTask;

    struct Value
    {
        TimerTask*  timerTask;
        TimeSpan    period;

        Value() :
            timerTask(0),
            period(0)
        {
        }

        Value(TimerTask* timerTask, TimeSpan period) :
            timerTask(timerTask),
            period(period)
        {
        }
    };

    Tree<DateTime, Value>   queue;
    es::Thread*             thread;
    es::Monitor*            monitor;
    bool                    canceled;

    static void* run(void* param)
    {
        Timer* timer = static_cast<Timer*>(param);
        return timer->start();
    }

    void* start()
    {
        while (!canceled)
        {
            TimerTask* task;
            while (!canceled)
            {
                Synchronized<es::Monitor*> method(monitor);
                Tree<DateTime, Value>::Node* node(queue.getFirst());
                if (node)
                {
                    TimeSpan diff = node->getKey() - DateTime::getNow();
                    task = node->getValue().timerTask;
                    if (0 < diff)
                    {
                        monitor->wait(diff);
                    }
                    else
                    {
                        queue.remove(node->getKey());
                        break;
                    }
                }
                else
                {
                    monitor->wait(10000000);
                }
            }
            if (!canceled)
            {
                if (task->period == 0)
                {
                    task->executionTime = 0;
                }
                task->run();
                if (0 < task->period)
                {
                    schedule(task,
                             task->scheduledExecutionTime() + task->period,
                             task->period);
                }
            }
        }
        thread = 0;
        monitor->notifyAll();
    }

public:
    Timer() :
        thread(0),
        canceled(false)
    {
        monitor = es::Monitor::createInstance();
        thread = esCreateThread(run, this);
        thread->setPriority(es::Thread::Highest);
        thread->start();
        thread->release();
    }

    ~Timer()
    {
        canceled = true;
        monitor->notifyAll();
        {
            Synchronized<es::Monitor*> method(monitor);

            while (thread)
            {
                monitor->wait();
            }
        }
        monitor->release();
    }

    void schedule(TimerTask* timerTask, DateTime time)
    {
        Synchronized<es::Monitor*> method(monitor);

        Value v(timerTask, 0);
        timerTask->period = 0;
        timerTask->executionTime = time;

        bool done = false;
        do
        {
            try
            {
                queue.add(timerTask->executionTime, v);
                done = true;
            }
            catch (SystemException<EEXIST>)
            {
                timerTask->executionTime.addTicks(1);
            }
        } while (!done);

        monitor->notifyAll();
    }

    void schedule(TimerTask* timerTask, DateTime firstTime, TimeSpan period)
    {
        Synchronized<es::Monitor*> method(monitor);

        Value v(timerTask, period);
        timerTask->period = period;
        timerTask->executionTime = firstTime;

        bool done = false;
        do
        {
            try
            {
                queue.add(timerTask->executionTime, v);
                done = true;
            }
            catch (SystemException<EEXIST>)
            {
                timerTask->executionTime.addTicks(1);
            }
        } while (!done);

        monitor->notifyAll();
    }

    void schedule(TimerTask* timerTask, TimeSpan delay)
    {
        schedule(timerTask, DateTime::getNow() + delay);
    }

    void schedule(TimerTask* timerTask, TimeSpan delay, TimeSpan period)
    {
        schedule(timerTask, DateTime::getNow() + delay, period);
    }

    void cancel(TimerTask* timerTask)
    {
        Synchronized<es::Monitor*> method(monitor);
        if (timerTask->isEnabled())
        {
            queue.remove(timerTask->executionTime);
            timerTask->executionTime = 0;
            timerTask->period = 0;  // Stop periodic timer
        }
    }
};

#endif  // NINTENDO_ES_TIMER_H_INCLUDED
