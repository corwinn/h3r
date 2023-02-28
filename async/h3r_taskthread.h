/**** BEGIN LICENSE BLOCK ****

BSD 3-Clause License

Copyright (c) 2021-2023, the wind.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**** END LICENCE BLOCK ****/

#ifndef _H3R_TASKTHREAD_H_
#define _H3R_TASKTHREAD_H_

#include "h3r.h"
#include "h3r_os.h"
#include "h3r_thread.h"
#include "h3r_criticalsection.h"
#include "h3r_iasynctask.h"

H3R_NAMESPACE

// Starts its own thread that does IAsyncTask::Do() - one at a time.
// Usage:
//   class Bar : public IAsyncTask
//   {
//      void Do() override {}
//   } foo;
//   TaskThread _a;
//   _a.Task = foo;
// The task is done once Done() returns true. Setting another task prior the
// current one is Done() will result in an exit() with an assertion failed.
// 00:48:00
class TaskThread final
{
    H3R_CANT_COPY(TaskThread)
    H3R_CANT_MOVE(TaskThread)

    public TaskThread() : _tproc {*this}, _thr {_tproc}, Task {*this} {}
    public ~TaskThread() { _thr.Stop (); };

#undef public
    private struct Proc final : public OS::Thread::Proc
#define public public:
    {
        inline Proc * Run() override { return _a.Run (); }
        TaskThread & _a;
        Proc(TaskThread & a) : _a {a}, _dirty {*this}  {}
        IAsyncTask * _p {};
        OS::CriticalSection _d_lock {}, _op_lock {};
        public class BoolProperty final // bool foo { get {} set {} }
        {
            private bool _v {};
            private Proc & _p;
            private BoolProperty(Proc & p) : _p {p} {}
            friend struct Proc;
            public inline bool & operator=(const bool & v)
            {
                __pointless_verbosity::CriticalSection_Acquire_finally_release
                    ____ {_p._d_lock};
                return _v = v;
            }
            public operator bool() const { return _v; }
            public bool operator !() const { return !_v; }
        } _dirty;
        // setup part
        inline IAsyncTask & Do(IAsyncTask * p)
        {
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ____ {_op_lock};
            H3R_ENSURE(! _dirty, "Another task in progress")
            _p = p;
            _dirty = true;
            return *p;
            // OS::Log_stdout ("p:%p, dirty:%d" EOL, this, (_dirty ? 1 : 0));
        }
        // Bridge part
        inline void Do() { _p->Do (); }
    } _tproc;
    private OS::Thread _thr;
    private template <typename TF> struct try_finally_trig
    {// instead of try {} finally {}
        TF & _v;
        try_finally_trig(TF & v) : _v {v} {}
        ~try_finally_trig() { _v = !_v; }
    };
    private struct try_finally_null
    {// instead of try {} finally {}
        TaskThread * & _v;
        try_finally_null(TaskThread * & v) : _v {v} {}
        ~try_finally_null() { _v = nullptr; }
    };
    private inline void Do() // part of Run() below
    {
        try_finally_trig<typename Proc::BoolProperty> ____ {_tproc._dirty};
        try_finally_null ___ {_tproc._p->_TT = this};//TODO de-mishmash-ise
        _tproc.Do ();
        // without the pointless verbosity above:
        //  try { _tproc.Do (); } finally { _tproc._dirty = ! _tproc._dirty; }
    }
    private inline Proc * Run() // the thread
    {
        while (! _tproc.stop) {
            // OS::Log_stdout ("p:%p, _tproc.dirty:%d" EOL,
            //    &_tproc, (_tproc._dirty ? 1 : 0));
            if (_tproc._dirty) Do (); // laundry
            else OS::Thread::SleepForAWhile ();
        }
        return &_tproc;
    }

    private inline IAsyncTask & Do(IAsyncTask * it) { return _tproc.Do (it); }

    public class TaskProperty final // IAsyncTask foo { set {} }
    {
        private TaskThread & _a;
        private TaskProperty(TaskThread & a) : _a {a} {}
        friend class TaskThread;
        public inline IAsyncTask & operator=(const IAsyncTask & t)
        {
            H3R_ENSURE(nullptr == t._TT, "1 thread per task please")
            return _a.Do (const_cast<class IAsyncTask *>(&t));
        }
    } Task;

    public inline bool Done() const { return ! _tproc._dirty; }
};// TaskThread

NAMESPACE_H3R

#endif