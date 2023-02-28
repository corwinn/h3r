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

#ifndef _H3R_OS_STDIO_WRAPPERS_H_
#define _H3R_OS_STDIO_WRAPPERS_H_

// File IO is anything but simple.

// fread and fwrite wrappers.

#include <stdio.h>
#include <sys/stat.h>

#include "h3r.h"
#include "h3r_os_error.h"

H3R_NAMESPACE
namespace OS {

// Threading: the file IO will be using a thread != "main". This means you'll
// have to put this thread on hold while waiting for the user of your nice GUI
// to resolve the issue. Terminating the program at this specific moment can
// not be handled gracefully w/o breaking the contract defined by the wrappers
// below, or w/o enabling the C++ exceptions. No matter what I do, there will
// be data loss - so please inform the user to not close the program while the
// file IO is asking important questions. This all goes away when using the
// default policy, which is exit() on IO error - leads to data loss as well.
// The most probable no-loss solution is for the user to pay attention and
// resolve the issue, when possible.
// Also, resolving an issue for a file log, for too long, can lead to buffer
// overflow at the Log Service - leads to exit().
//
// What you need to do:
//  1. Inherit Error and init Error::File with your descendant.
//     Now it shall receive instance of this class via "e".
// What can the user do:
// Backup your data.
//  - on Relace*           : ask the user to confirm file replacement
//                           set e->Replace to the user's choice
// Exclude Replace from the following list of "-".
//  - on WriteNoSpace      : free some space
//  - on *NoPermission     : grant permission(s)
//  - on *Physical         : backup your data; retry on your own responsibility
//  - on *NoDev            : device lost - if it was an USB plug it back in
//  - on *NoMem            : free some RAM
//  - on ReadNoSpace       : a bug: call the file system programmer
//  - on SeekNoSpace       : see WriteNoSpace
//  - on OpenNoSpace       : see WriteNoSpace
//  - on OpenLoop          : resolve the symbolic links loop
//  - on OpenNameTooLong   : choose short-er path
//  - on anything else     : backup your data
// Important - it will retry REASONABLE_RETRIES (defined at the .cpp) at most.
// When there is a physical error: a retry can take any amount of time.
// When there is a physical error: backup 1st, retrying is optional.
// When there is a physical error: retrying is dangerous.
// On "windows", freeing resources can be tricky - there is a special kernel
// cache that can get exhausted - you can't free anything there. So if you get
// retry after you freed some RAM - you're in for a reboot. This can be avoided
// by using CreateFile(FILE_FLAG_NO_BUFFERING) instead of fopen() - so perhaps
//TODO there will be a command line option addressing it.
#undef public
class FileError: public Error
#define public public:
{
    protected bool Handled(Error * e = nullptr) override final;
    public enum class Op {Read, Write, Seek, Open, Close, Replace} Op;
    public enum class Code {NoPermission, NoSpace, Physical, NoDev,
        Loop, NameTooLong, NoMem} Code;
    public FileError(enum Op o, enum Code c) : Op {o}, Code {c} {}
    public bool Replace {false};
};

// These functions are designed to not fail. They will
// exit(EXIT_ASSERTION_FAILED) if unrecoverable or unhandled error occurs.
// This means reading beyond EOF won't work for example.
// You could be given a choice to retry the last operation - see FileError
// above.

#define H3R_FILE_OP_NOT_SUPPORTED -1

// Can return H3R_FILE_OP_NOT_SUPPORTED ("stdout" for example).
// Returns 0 on success, or not at all.
int   Fseek (FILE * stream, off_t offset, int whence);
// Can return H3R_FILE_OP_NOT_SUPPORTED ("stdout" for example).
// Returns rw file pointer on success, or not at all.
off_t Ftell (FILE * stream);
// Reading from write-only streams is not recommended.
void  Fread (void * ptr, size_t size, size_t nmemb, FILE * stream);
// Writing to read-only streams is not recommended.
void  Fwrite(const void * ptr, size_t size, size_t nmemb, FILE * stream);
//
FILE * Fopen(const char * path, const char * mode);
//
void Fclose(FILE *stream);

// Not wrapper, but helper
bool FileExists(const char * path);
//LATER full stat() wrapper; requires creating struct Stat {foo;bar;etc};
//      take into consideration: Exists doesn't require user intervention
//      while other stats might
off_t FileSize(const char * path);

} // namespace OS
NAMESPACE_H3R

#endif