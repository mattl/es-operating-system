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

#ifndef NINTENDO_ES_TIMER_H_INCLUDED
#define NINTENDO_ES_TIMER_H_INCLUDED

#include <new>
#include <es/clsid.h>
#include <es/dateTime.h>
#include <es/synchronized.h>
#include <es/timeSpan.h>
#include <es/tree.h>
#include <es/base/IMonitor.h>
#include <es/base/IThread.h>

IThread* esCreateThread(void* (*start)(void* param), void* param);

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
    IThread*                thread;
    IMonitor*               monitor;
    bool                    canceled;

    static void* run(void* param)
    {
        Timer* timer = static_cast<Timer*>(param);
        return timer->start();
    }

    void* start()
    {
        Synchronized<IMonitor*> method(monitor);

        while (!canceled)
        {
            Tree<DateTime, Value>::Node* node(queue.getFirst());
            if (node)
            {
                TimeSpan diff = node->getKey() - DateTime::getNow();
                TimerTask* task = node->getValue().timerTask;
                if (0 < diff)
                {
                    monitor->wait(diff);
                }
                else
                {
                    queue.remove(node->getKey());
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
            else
            {
                monitor->wait(10000000);
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
        esCreateInstance(CLSID_Monitor,
                         IID_IMonitor,
                         reinterpret_cast<void**>(&monitor));
        thread = esCreateThread(run, this);
        thread->setPriority(IThread::Highest);
        thread->start();
        thread->release();
    }

    ~Timer()
    {
        canceled = true;
        monitor->notifyAll();
        {
            Synchronized<IMonitor*> method(monitor);

            while (thread)
            {
                monitor->wait();
            }
        }
        monitor->release();
    }

    void schedule(TimerTask* timerTask, DateTime time)
    {
        Synchronized<IMonitor*> method(monitor);

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
        Synchronized<IMonitor*> method(monitor);

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
        Synchronized<IMonitor*> method(monitor);
        if (timerTask->isEnabled())
        {
            queue.remove(timerTask->executionTime);
            timerTask->executionTime = 0;
            timerTask->period = 0;  // Stop periodic timer
        }
    }
};

#endif  // NINTENDO_ES_TIMER_H_INCLUDED
