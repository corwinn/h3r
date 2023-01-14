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

#ifndef _H3R_THREAD_H_
#define _H3R_THREAD_H_

#include "h3r_os.h"
#include <pthread.h>

H3R_NAMESPACE
namespace OS {

class Thread final
{
    // Extend this with thread-specific state.
    public: struct Proc
    {
        bool stop {false};
        virtual Proc * Run() { return this; }
    };
    private: Proc & _p;
    private: pthread_t _thr {};
    // public: static Thread & Create(Proc &);
    public: static void Sleep(int); // [milliseconds] : [1;1000]
    public: Thread(Proc &);
    public: ~Thread();
    // Wait for the thread to stop.
    public: void Join();
    // Set Proc.stop to true and wait for the thread to stop.
    public: void Stop();

    private: static Thread * Threads[];
    // Signal all threads to stop and wait for them to stop.
    public: static void StopAll();
}; // class Thread

} // namespace OS
NAMESPACE_H3R

#endif
