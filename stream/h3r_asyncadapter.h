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

// You unwind the faucet, and you wait for the hot water.

#ifndef _H3R_ASYNCADAPTER_H_
#define _H3R_ASYNCADAPTER_H_

#include "h3r.h"
#include "h3r_os.h"
#include "h3r_taskthread.h"
#include "h3r_iasynctask.h"
#include "h3r_stream.h"
#include "h3r_criticalsection.h"

H3R_NAMESPACE

// Unqueued async. stream adapter. T - your class - the one that handles
// on_complete and on_canceled.
//
// In "async.", function parameters become record fields become shared
// state => (complexity++)*=(complexity++). Your program however, becomes
// more and more elegant.
//
// The game will do all of its file and network IO through this adapter.
// Usage:
//   class foo
//   {
//      AsyncAdapter<foo> _a {thr, this, &foo::complete, &foo::canceled};
//      void complete(AsyncAdapter<foo>::Result) {}
//      void canceled(AsyncAdapter<foo>::Result) {}
//   };
//   _a.Stream = soo;  - you don't have to set the same stream prior each op;
//                       your stream is not being copied - don't worry;
//   _a.Read (&bar, 10); - your thread won't wait for the read() to complete;
//   &bar will be usable when "complete" is called.
//   The handlers above will be called in the IO thread context.
//   Do not do async ops inside the handlers above.
//   One async op at a time, only! - e.g. the "Unqueued" part.
//   Do not mess up your state (&bar and _a.Stream) until you get notified.
// Setting nullptr anywhere, or modifying the stream reference while an async
// op is in progress, will get you an exit() with an assertion failed.
// When the user chooses to cancel, you do _a.Cancel (true) in order to get the
// canceled() call. Do not block the IO thread on sync objects - let it loop
// with Thread::Sleep(1), while the user makes a choice.
// Using the async IO to read/write 1 byte at a time is a very bad idea - you
// could get FOR_AWHILE (taskthread.h) "lag" for each IO op.
// Use-case: the UI thread requests data from a file; it periodically checks
//           if the data is available or if there is a retry request, then
//           processes the UI; the file IO thread requests the user to confirm
//           retry-read; the UI thread displays a dialog asking the user's
//           opinion; the 2nd attempt succeeds; the UI thread renders the data
// Abbreviations: "eh" - event handler; "s" - stream; "r" - result;
template <typename T> class AsyncAdapter final
{
    public: enum class StreamOp {Read, Write, Seek, Tell, Size, OK};
    public: struct Result
    {
        enum StreamOp Op;
        off_t Pos, Size;
        bool OK; // if the stream is ok
    };
    using EventHandler = void (T::*)(const Result);

    private: TaskThread & _async;
    public: AsyncAdapter(TaskThread & thread, T * eh_obj,
        EventHandler on_complete, EventHandler on_canceled)
        : _async {thread}, _eh_obj {eh_obj}, _eh_complete {on_complete},
        _eh_canceled {on_canceled}, Stream {*this}, _task {*this}
    {
        H3R_ENSURE(nullptr != _eh_obj, "eh_obj can't be null")
        H3R_ENSURE(nullptr != _eh_complete, "_eh_complete can't be null")
        H3R_ENSURE(nullptr != _eh_canceled, "_eh_canceled can't be null")
    }
    private: OS::CriticalSection _s_in_use_lock {};
    private: bool _s_in_use {};
    private: Stream * _s {};
    private: T * _eh_obj {};
    private: EventHandler _eh_complete {};
    private: EventHandler _eh_canceled {};

    public: class StreamProperty { // public Stream Stream { set {} }
        private: AsyncAdapter<T> & _a;
        private: StreamProperty(AsyncAdapter<T> & a) : _a {a} {}
        friend class AsyncAdapter<T>;
        public: inline Stream & operator=(const Stream & s)
        {
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ____ {_a._s_in_use_lock};
            H3R_ENSURE(! _a._s_in_use, "Do not modify my stream property"
                " while an async op is in progress")

            return *(_a._s = const_cast<class Stream *>(&s));
        }
    } Stream;

    // The prefix "Thr" marks the methods TaskThread context.
    private: void ThrNotify(bool canceled)
    {
        if (canceled) (_eh_obj->*_eh_canceled) (_task._r);
        else          (_eh_obj->*_eh_complete) (_task._r);
    }
    private: inline bool ThrOK() { return _s->operator bool(); }
    private: inline void ThrSeek(off_t pos) { _s->Seek (pos); }
    private: inline off_t ThrTell() { return _s->Tell (); }
    private: inline off_t ThrSize() { return _s->Size (); }
    private: inline void ThrRead(void * b, size_t bytes = 1)
    {
        _s->Read (b, bytes);
    }
    private: inline void ThrWrite(const void * b, size_t bytes = 1)
    {
        _s->Write (b, bytes);
    }
    private: inline void Do() { _async.Task = _task; }

    private: class StreamTask final : public IAsyncTask
    {
        private: AsyncAdapter<T> & _a;
        private: StreamTask(AsyncAdapter<T> & a) : _a {a} {}
        friend class AsyncAdapter<T>;
        private: Result _r;
        private: off_t _pos {};         // param: Seek()
        private: void * _buf {};        // param: Read()
        private: const void * _cbuf {}; // param: Write()
        private: size_t _bytes {};      // param: Read(), Write()
        private: bool _cancel {}, // param: ThrNotify()
                      _dirty {}; // sentinel: 1 task at a time
        private: OS::CriticalSection _op_lock {};
        private: struct try_finally_trig final
        {// c++ try {} finally {}
            bool & _v;
            try_finally_trig(bool & v) : _v {v} {}
            ~try_finally_trig() { _v = !_v; }
        };
        public: inline void Do() override // TaskThread context
        {
            try_finally_trig ____ {_dirty};
            switch (_r.Op) {
                case StreamOp::Read: this->Read (); break;
                case StreamOp::Seek: this->Seek (); break;
                case StreamOp::Tell: this->Tell (); break;
                case StreamOp::Size: this->Size (); break;
                case StreamOp::Write: this->Write (); break;
                case StreamOp::OK: this->OK (); break;
                default: break;
            }
            _a.ThrNotify (_cancel);
        }
        // setup part
        public: inline void Do(StreamOp op)
        {
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ____ {_op_lock};
            H3R_ENSURE(! _dirty, "Op in progress")
            _dirty = true;
            _r.Op = op;
            _a.Do (); // notifies the TaskThread to invoke the above Do()
        }
        inline void DoOK() { Do (StreamOp::OK); }
        inline void DoSeek(off_t pos) { _pos = pos; Do (StreamOp::Seek); }
        inline void DoTell() { Do (StreamOp::Tell); }
        inline void DoSize() { Do (StreamOp::Size); }
        inline void DoRead(void * b, size_t bytes = 1)
        {
            _buf = b; _bytes = bytes; Do (StreamOp::Read);
        }
        inline void DoWrite(const void * b, size_t bytes = 1)
        {
            _cbuf = b; _bytes = bytes; Do (StreamOp::Write);
        }
        // Bridge part - TaskThread context
        inline void OK() { _r.OK = _a.ThrOK (); }
        inline void Seek() { _a.ThrSeek (_pos); }
        inline void Tell() { _r.Pos = _a.ThrTell (); }
        inline void Size() { _r.Size = _a.ThrSize (); }
        inline void Read() { _a.ThrRead (_buf, _bytes); }
        inline void Write() { _a.ThrWrite (_cbuf, _bytes); }
    } _task;

    public: inline void Cancel() { _task.cancel = true; }

    public: inline void OK() { _task.DoOK (); }
    public: inline void Seek(off_t pos) { _task.DoSeek (pos); }
    public: inline void Tell() { _task.DoTell (); }
    public: inline void Size() { _task.DoSize (); }
    public: inline void Read(void * b, size_t bytes = 1)
    {
        _task.DoRead (b, bytes);
    }
    public: inline void Write(const void * b, size_t bytes = 1)
    {
        _task.DoWrite (b, bytes);
    }
};

NAMESPACE_H3R

#endif