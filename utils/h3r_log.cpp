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

#include "h3r_log.h"

H3R_NAMESPACE

void Log::Swap() // _pq and _gq
{
    __pointless_verbosity::CriticalSection_Acquire_finally_release ___
        {_lock};
    if (&_q1 == _gq) _gq = &_q2, _pq = &_q1; else _gq = &_q1, _pq = &_q2;
}

Log::Log()
    : _pq {&_q1}, _gq {&_q2}, _thread_proc {*this}, _thr {_thread_proc}
{
    H3R_ENSURE(! Log::_one, "I'm a c++ singleton!")
    Log::_one = this;
    OS::Log_stdout ("Log::Log ()" EOL);
}

Log::~Log() { _thr.Stop (); _one = nullptr; }

void Log::Flush()
{
    int const FOR_AWHILE {10}; // [milliseconds]
    if (_gq->Empty ()) Swap ();
    if (_gq->Empty ()) { OS::Thread::Sleep (FOR_AWHILE); return; }

    __pointless_verbosity::CriticalSection_Acquire_finally_release ___
        {_ll_lock};
    if (! _thread_proc.silent && _listeners.Length () <= 0) {
        String msg {"Warning: The log service has no clients! "};
        _fall_back.Log (msg);
    }
    while (! _gq->Empty ()) {
        var msg = _gq->Dequeue ();
        if (! _thread_proc.silent)
            for (var l : _listeners) l->Log (msg);
    }
}

Log::Proc * Log::Run()
{
    while (! _thread_proc.stop)
        Flush ();
    Flush ();
    return &_thread_proc;
}

void Log::Subscribe(ILog * log_listener)
{
    H3R_ENSURE(nullptr != log_listener, "log_listener can't be null")
    for (var l : _listeners) {
        H3R_ENSURE(l != log_listener, "log_listener duplicate")
    }

    __pointless_verbosity::CriticalSection_Acquire_finally_release ___
        {_ll_lock};
    _listeners.Append (&log_listener, 1);
}

void Log::Silent(bool v) { _thread_proc.silent = v; }

void Log::info(String message) { log ("Info: " + message); }

/*static*/ void Log::Info(String message)
{
    if (Log::_one) Log::_one->info ((String &&)message);
    else {
        String msg {"Warning: Log service is off. Message: " + message};
        Log::_fall_back.Log (msg);
    }
}

NAMESPACE_H3R