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

#ifndef _H3R_TASKSTATE_H_
#define _H3R_TASKSTATE_H_

#include "h3r.h"
#include "h3r_string.h"

H3R_NAMESPACE

#define public public:
#define private private:
#define protected protected:

// These things should be set/get as one. Because threads. Thread A creates an
// immutable task state object. Thread B reads it, or not. Acquire/Release at
// one place - not for each field.
class TaskState
{
    private int _p;
    private String _s;
    public TaskState(TaskState && t) { operator= ((TaskState &&)t); }
    public inline TaskState & operator=(TaskState && t)
    {
        _s = static_cast<String &&>(t._s);
        _p = t._p;
        _changed = t._changed;
        return *this;
    }
    public TaskState(const TaskState & t) { operator= (t); }
    public inline TaskState & operator=(const TaskState & t)
    {
        _s = t._s;
        _p = t._p;
        _changed = t._changed;
        return *this;
    }

    // private TaskState * _t, * _tp {nullptr};
    public TaskState(int progress, String && message)
        //, TaskState * sub_task_state = nullptr)
        : _p {progress}, _s {static_cast<String &&>(message)}
        // ,_t{sub_task_state}
    {
        /*H3R_ENSURE(sub_task_state != this, "don't do that")
        if (nullptr != sub_task_state) {
            H3R_ENSURE(nullptr == sub_task_state->_tp, "_tp shall be null")
            sub_task_state->_tp = this;
        }*/
    }
    // When PercentageProgress() returns true: [0;100] [%].
    // Sometimes task completeness is unknown, like when enumerating FS.
    public inline const int & Progress() const { return _p; }
    public inline virtual bool PercentageProgress() const { return true; }
    // Brief description, like: "Loading ..."
    public inline const String & Message() const { return _s; }

    // The producer invokes SetChanged();
    // The consumer: Changed()
    // The producer shouldn't waste too much time with this.
    protected bool _changed {};
    public inline virtual void SetChanged(bool value) { _changed = value; }
    public inline virtual bool Changed()
    {
        var result = _changed;
        if (_changed) _changed = false;
        return result;
    }
    public inline static bool Changed(const TaskState & a, const TaskState & b)
    {
        return (a.Message () != b.Message ())
            || (a.Progress () != b.Progress ());
    }

    /* This doesn't seem to has a use-case at this project, so: to be deleted...
    // For nested tasks, like: enumerating and reading:
    // sub_task_state->Progress() is read progress.
    //LATER loop detection; depth limit (3 sounds reasonable); simple:
    //      a->b != a && a->b->c != a && a->b != a->b->c; (a & b & c) != null
    public: inline const TaskState * SubTaskState() const { return _t; }
    public: inline const TaskState * ParentTaskState() const { return _tp; }*/
    public static TaskState Unknown;
};// TaskState

#undef public
#undef private
#undef protected

NAMESPACE_H3R

#endif