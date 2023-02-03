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

#ifndef _H3R_OS_H_
#define _H3R_OS_H_

#if _WIN32
#include "windows.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#include "h3r.h"
#include "h3r_os_error.h"
#ifdef H3R_MM
# include "h3r_mm.h"
#endif
#include "h3r_criticalsection.h"

//TODO EOL for mac
#ifndef EOL
# if _WIN32
#  define EOL "\r\n"
# else
#  define EOL "\n"
# endif
#endif

#if _WIN32
# define H3R_PATH_SEPARATOR '\\'
#else
# define H3R_PATH_SEPARATOR '/'
#endif

// way easier to spot in code
#define H3R_OS_GLOBAL_ERR_CODE errno

H3R_NAMESPACE
namespace OS {

int const EXIT_NO_ERROR         = 0;
int const EXIT_WITH_ERROR       = 1;
int const EXIT_OUT_OF_MEMORY    = 2;
int const EXIT_CANT_LOG         = 3;
int const EXIT_ASSERTION_FAILED = 4;
int const EXIT_STREAM_ERROR     = 5;

auto Exit = [](int status) __attribute__((__noreturn__)) { exit (status); };

void Log_stderr(const char *, ...);
#define H3R_LOG_STDERR(M,...) \
    H3R_NS::OS::Log_stderr ("Fixme: %s:%d: ", __FILE__, __LINE__), \
    H3R_NS::OS::Log_stderr (M EOL, __VA_ARGS__);
// Warning - this is incompatible with unit testing!
#define H3R_ENSURE(C,M) if (! (C)) { \
    H3R_LOG_STDERR("Assertion Failed: %s", M) \
    H3R_NS::OS::Exit (H3R_NS::OS::EXIT_ASSERTION_FAILED); \
    }

// Your program gets terminated if stderr is not available.
inline void Log_stderr(const char * fmt, ...)
{
#ifdef H3R_DEBUG
    static CriticalSection thread_gate {};
    int r {};
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release ____ {
            thread_gate};

        va_list ap;
        va_start (ap, fmt);
        r = vfprintf (stderr, fmt, ap);
        va_end (ap);
    }
    if (r < 0) exit (EXIT_CANT_LOG);
#else
    (void)fmt;
#endif
};

// Your program gets terminated if stdout is not available.
// H3R_TEST usage.
auto Log_stdout = [](const char * fmt, ...)
{
    static CriticalSection thread_gate {};
    int r {};
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release ____ {
            thread_gate};
        va_list ap;
        va_start (ap, fmt);
        r = vprintf (fmt, ap);
        va_end (ap);
    }
    if (r < 0) exit (EXIT_CANT_LOG);
};

auto Strlen = [](const char * str) { return strlen (str); };

auto Strncmp = [](const char * a, const char * b, size_t n)
{
    return strncmp (a, b, n);
};

auto Memmove = [](void * dest, const void * src, size_t n)
{
    memmove (dest, src, n);
};

auto Memcpy = [](void * dest, const void * src, size_t n)
{
    memcpy (dest, src, n);
};

auto Memset = [](void * s, int c, size_t n)
{
    memset (s, c, n);
};

auto ToLower = [](int c) { return tolower (c); };

// These are the actual malloc/free bridge - way faster than ManagedMemory.
template <typename T> void Malloc(T *& p, size_t n = 1)
{
    H3R_ENSURE(n > 0, "n < 1")
    byte r = 1; // "number of retries" = "number of bits" - 1
                // Infinite loop protection.
    do
        p = (T *)calloc (n, sizeof(T));
    while (! p && Error::Memory.Handled () && (r = r << 1));
    if (! p)
        Exit (EXIT_OUT_OF_MEMORY);
}
template <typename T> void Mfree(T * & p) { if (p) free (p), p = nullptr; }

#ifdef H3R_MM
# include "h3r_mm_templates.h"
template <typename T> void Alloc(T * & p, size_t n = 1) { MM::Alloc (p, n); }
template <typename T> void Free(T * & p) { MM::Free (p); }
#else
template <typename T> void Alloc(T * & p, size_t n = 1) { Malloc (p, n); }
template <typename T> void Free(T * & p) { Mfree (p); }
#endif

// Since file enum is used at startup only, there is no need to play the retry
// game here.
namespace {
    template <typename T> void EnumFiles_err(T e, const char * p)
    {
        Log_stderr ("%s(): ", p);
        switch (e) {
            case EACCES: Log_stderr ("access denied"); break;
            case EMFILE: Log_stderr ("process: too many open files"); break;
            case ENFILE: Log_stderr ("OS: too many open files"); break;
            case ENOENT: Log_stderr ("path not found"); break;
            case ENOMEM: Log_stderr ("out of memory"); break;
            case ENOTDIR: Log_stderr ("not a directory"); break;
            case EIO   : Log_stderr ("IO error") ; break;
            case ELOOP : Log_stderr ("symlink loop") ; break;
            case ENAMETOOLONG: Log_stderr ("name too long") ; break;
            default: Log_stderr ("unknown error: %d", e); break;
        }
        Log_stderr (EOL);
    }
}
namespace __pointless_verbosity
{
    struct try_finally_closedir
    {// instead of "try { ... } finally { ... }"
        DIR * d;
        try_finally_closedir(DIR * v) : d(v) {}
        ~try_finally_closedir() { if (d) closedir (d), d = 0; }
    };
    struct try_finally_mfree
    {
        void * d;
        try_finally_mfree(void * v) : d(v) {}
        ~try_finally_mfree() { Mfree (d); }
    };
    template <typename T> struct __try_finally_free
    {
        T * _state;
        __try_finally_free(T * state) : _state {state} {}
        ~__try_finally_free() { Free (_state); }
    };
}
// Enumerate all files at directory "dn", in a non-recursive manner.
template <typename T> bool EnumFiles(T & c, const char * dn,
    bool (*on_file)(T & c, const char * n, bool directory))
{
    H3R_ENSURE(nullptr != on_file, "on_file can't be null")
    H3R_ENSURE(nullptr != dn, "path name can't be null")
    auto stat_dlen = strlen (dn);
    H3R_ENSURE(stat_dlen > 0, "path name length can't be <= 0")
    if (H3R_PATH_SEPARATOR == dn[stat_dlen-1]) stat_dlen--;

    DIR * ds = opendir (dn);
    if (nullptr == ds)
        return EnumFiles_err (H3R_OS_GLOBAL_ERR_CODE, "opendir"), false;
    __pointless_verbosity::try_finally_closedir ____ {ds};
    char * stat_name;                       // stat() requirements
    int stat_name_max_size {4096};          //TODO stat() should go elsewhere
    Malloc (stat_name, stat_name_max_size);
    __pointless_verbosity::try_finally_mfree ___ {stat_name};
    for (dirent * de = nullptr;;) {
        const auto e = H3R_OS_GLOBAL_ERR_CODE;
        de = readdir (ds);
        if (nullptr == de) {
            const auto e2 = H3R_OS_GLOBAL_ERR_CODE;
            return e != e2 ? EnumFiles_err (e2, "readdir"), false : true;
        }
        if (! de->d_name[0]) {
            Log_stdout ("Warning: readdir(): empty d_name");
            continue;
        }
        auto len = strlen (de->d_name);
        if (1 == len && '.' == de->d_name[0]) continue;
        if (2 == len && '.' == de->d_name[0]
                     && '.' == de->d_name[1]) continue;
        struct stat finfo;

        // dn + H3R_PATH_SEPARATOR + d_name
        int stat_clen = stat_dlen + 1 + len + 1; // +1 -> PS, +1; -> '\0'
        if (stat_clen > stat_name_max_size)
            Malloc (stat_name, stat_name_max_size = stat_clen);
        Memmove (stat_name, dn, stat_dlen);
        stat_name[stat_dlen] = H3R_PATH_SEPARATOR;
        Memmove (stat_name + stat_dlen + 1, de->d_name, len); // +1 -> PS
        stat_name[stat_clen-1] = '\0'; // +1 computed above

        // Log_stdout ("stat(): \"%s\"" EOL, stat_name);
        if (-1 == stat (stat_name, &finfo))
            return EnumFiles_err (H3R_OS_GLOBAL_ERR_CODE, "stat"), false;
        bool dir = S_ISDIR(finfo.st_mode);
        if (! dir && ! S_ISREG(finfo.st_mode)) {
            Log_stdout ("Warning: readdir(): skipped ! file && ! directory:"
                " \"%s%c%s\"" EOL, dn, H3R_PATH_SEPARATOR, de->d_name);
            continue;
        }
        if (! on_file (c, de->d_name, dir)) break;
    } // for (;;)
    return true;
} // EnumFiles

//LATER - per OS - get_process_path

} // namespace OS
NAMESPACE_H3R

#endif