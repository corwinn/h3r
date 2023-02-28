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

#ifndef _H3R_OS_ERROR_H_
#define _H3R_OS_ERROR_H_

#define H3R_ERR_HANDLER_UNHANDLED H3R_NS::OS::Error::Unhandled
#define H3R_ERR_DEFINE_UNHANDLED H3R_NS::OS::Error H3R_ERR_HANDLER_UNHANDLED;
#define H3R_ERR_DEFINE_HANDLER(H, R) \
    H3R_NS::OS::Error & H3R_NS::OS::Error::H = R;

#include "h3r.h"

H3R_NAMESPACE

#ifdef H3R_TEST
#define H3R_THROW(E, M) throw E {__FILE__, __LINE__, M};
class Exception
{
    private const char * _file;
    private int _line;
    private const char * _msg;
    public Exception(const char * file, int line, const char * msg)
        : _file {file}, _line {line}, _msg {msg} {}
    public inline const char * File() const { return _file; }
    public inline int Line() const { return _line; }
    public inline const char * Msg() const { return _msg; }
};
// Show when things get called with invalid arguments.
#undef public
class ArgumentException : public Exception
{
    public: using Exception::Exception;
};
// Show that some operation isn't supported.
class NotSupportedException : public Exception
{
    public: using Exception::Exception;
};
// Remind that some method isn't implemented.
// Way better than the pure virtual ...
class NotImplementedException : public Exception
{
    public: using Exception::Exception;
};
#define public public:

// H3R_TEST
#else
#define H3R_THROW(E, M) H3R_ENSURE(false, "Unhandled Exception " #E ": " M)
#endif

#define H3R_THROW_IF(C, E, M) { if ((C)) H3R_THROW(E, M) }
#define H3R_ARG_EXC_IF(C, M) H3R_THROW_IF(C,ArgumentException, M)
#define H3R_NOT_SUPPORTED_EXC(M) H3R_THROW(NotSupportedException, M)
#define H3R_NOT_IMPLEMENTED_EXC \
    H3R_THROW(NotImplementedException, "Implement me.")

namespace OS {

class Error
{
    // Return true in order to try the failed op again.
    // e is optional specific error: ErrorFileNoSpace for example:
    //   if (Error::File.Handled (file_no_space)) ...
    // If e != nullptr, it returns e->Handled (nullptr); false otherwise.
    public virtual bool Handled(Error * e = nullptr);
    public static Error Unhandled;
    public static Error & Memory;
    public static Error & File;
    public static Error & Log;
};

} // namespace OS

NAMESPACE_H3R

#endif