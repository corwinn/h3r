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

// Polling async. dual-FIFO message dispatcher.

#ifndef _H3R_LOG_H_
#define _H3R_LOG_H_

#include "h3r.h"
#include "h3r_ilog.h"
#include "h3r_string.h"
#include "h3r_thread.h"
#include "h3r_criticalsection.h"
#include "h3r_log_stdout.h"

H3R_NAMESPACE

class Game;

// One instance only, please. Or you will get race conditions should you decide
// to register one listener at many log services.
//
// Its a complex web of short and simple - it shall stay this way.
class Log final
{
    H3R_CANT_COPY(Log)
    H3R_CANT_MOVE(Log)

    private: class
    {
        private: Array<String> _buf {256}; // ring
        private: int _p {}, _g {}, _c {};  // put ptr, get ptr, count
        public: inline void Enqueue(String && s)
        {//TODO a file log can cause your program to terminate - should the
// IO take too much time, this buffer overflow probability skyrockets;
// so make sure that your ILog is fast. Perhaps a timeout is ok here as well?
// Example: a file write fails and the user is being asked to free some space -
// it can take any amount of time.
// Or, perhaps this buffer should use the _fall_back or block until there is
// space? The thread context here is the one that wants to log a message.
            H3R_ENSURE(_c < (int)_buf.Length (),
                "Do I look like log4j or something?")
            _buf[_p] = s; _c++;
            _p = (_p + 1) % _buf.Length ();
        }
        public: inline String Dequeue()
        {
            H3R_ENSURE(! Empty (), "Nothing to dequeue.")
            var d = _g;
            return _g = (_g + 1) % _buf.Length (), _c--, (String &&)_buf[d];
        }
        public: inline bool Empty() { return 0 == _c; }
    } _q1, _q2, * _pq, * _gq; // dual FIFO: put queue, get queue
    private: OS::CriticalSection _lock; // dual FIFO lock
    private: OS::CriticalSection _ll_lock; // log listeners lock
    private: void Swap(); // _pq and _gq
    private: struct Proc final : public OS::Thread::Proc
    {
        Log & _log;
        Proc(Log & l) : _log {l} {}
        inline Proc * Run() override { return _log.Run (); } // just forward
        bool silent {};
    } _thread_proc;
    private: OS::Thread _thr; // thread obj
    private: Log();
    friend class H3R_NS::Game;
    //TODO find out what a bad block write error, for example, can cause to
    // pthread_join, for example; will it get interrupted? - the thread -
    // stopped? - etc.
    private: ~Log(); // will Flush() - be wary
    private: Array<ILog *> _listeners;

    private: void Flush();
    private: Proc * Run();

    //TODO if (_thread_proc.stop) ?!
    private: inline void log(String && s)
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release ___
            {_lock};
        _pq->Enqueue ((String &&)s);
    }

    // Warning! Bad things happen should the ILog gets destroyed prior the log
    // thread had stopped.
    //TODONT optimize _listeners duplicate search; this function is
    //       supposed to be called ~5 times at program start
    public: void Subscribe(ILog * log_listener);
    public: void Silent(bool);

    private: void info(String message);

    //DONE class Game - static storage duration: inside it: order MM, Log, etc.
    private: static Log * _one;
    private: static Log_Stdout _fall_back;
    public: static void Info(String message);
}; // class Log final

#define H3R_LOG_STATIC_INIT H3R_NS::Log_Stdout H3R_NS::Log::_fall_back; \
                            H3R_NS::Log * H3R_NS::Log::_one {nullptr};

NAMESPACE_H3R

#endif