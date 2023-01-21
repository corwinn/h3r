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

#include <errno.h>
#include "h3r_os_stdio_wrappers.h"
#include "h3r_os_error.h"
#include "h3r_os.h"

//TODO test me; simulate error code, somehow

H3R_NAMESPACE
namespace OS {

// One thread is acting here != "main", so no need for a thread gate.
// As a probable Log client, it can't log its issues via the log service.
//
// This is a work in progress. There are many unknowns yet. Consider this
// quick and dirty code. You have been warned.

static void ClearErr(FILE * f)
{
    if (ferror (f) || feof (f)) clearerr (f);
}

bool FileError::Handled(Error *) { return false; }

static int const REASONABLE_RETRIES {5};

template <typename T> static enum FileError::Code FECode(T e)
{
    switch (e) {
        case ENOSPC: return FileError::Code::NoSpace;
        case EINVAL: return FileError::Code::NoPermission;
        case EIO   : return FileError::Code::Physical;
        case ENXIO : return FileError::Code::NoDev;
        case ELOOP : return FileError::Code::Loop;
        case ENAMETOOLONG: return FileError::Code::NameTooLong;
        case ENOMEM: return FileError::Code::NoMem;
        default: Log_stderr ("  FileError: unhandled error code: %d" EOL, e);
    }
    H3R_ENSURE(false, "Unrecoverable file error")
}

template <typename T> static bool RetryWrite(T e)
{
    Log_stderr ("RetryWrite" EOL);
    FileError t {FileError::Op::Write, FECode (e)};
    return Error::File.Handled (&t);
}

template <typename T> static bool RetryRead(T e)
{
    Log_stderr ("RetryRead" EOL);
    FileError t {FileError::Op::Read, FECode (e)};
    return Error::File.Handled (&t);
}

template <typename T> static bool RetrySeek(T e)
{
    Log_stderr ("RetrySeek" EOL);
    FileError t {FileError::Op::Seek, FECode (e)};
    return Error::File.Handled (&t);
}

template <typename T> static bool RetryOpen(T e)
{
    Log_stderr ("RetryOpen" EOL);
    FileError t {FileError::Op::Open, FECode (e)};
    return Error::File.Handled (&t);
}

template <typename T> static bool RetryClose(T e)
{
    Log_stderr ("RetryClose" EOL);
    FileError t {FileError::Op::Close, FECode (e)};
    return Error::File.Handled (&t);
}

off_t Ftell(FILE * stream)
{
    H3R_ENSURE (nullptr != stream, "can't tell() at null stream")

    var t = ftello (stream);
    if (-1 == t) {
        const var e = H3R_OS_GLOBAL_ERR_CODE;
        if (ESPIPE == e) return ClearErr (stream), H3R_FILE_OP_NOT_SUPPORTED;
        Log_stderr ("  Ftell: unhandled error code: %d" EOL, e);
        H3R_ENSURE(false, "ftell() can't error")
    }
    return t;
}

int Fseek(FILE * stream, off_t offset, int whence)
{
    H3R_ENSURE (nullptr != stream, "can't seek() at null stream")
    H3R_ENSURE (   SEEK_SET == whence
                || SEEK_CUR == whence
                || SEEK_END == whence, "unknown whence is forbidden")

    if (SEEK_SET == whence && Ftell (stream) == offset) return 0;
    if (SEEK_CUR == whence && 0 == offset) return 0;

    for (int i = 0; i < REASONABLE_RETRIES; i++) {
        const var r = fseeko (stream, offset, whence);
        if (0 == r) return r;
        const var e = H3R_OS_GLOBAL_ERR_CODE;
        if (ESPIPE == e) return H3R_FILE_OP_NOT_SUPPORTED;
        ClearErr (stream);
        if (! RetrySeek (e)) break;
    }
    H3R_ENSURE(false, "Unrecoverable fseek error")
}

template <typename T> void RW(size_t (*p)(T, size_t, size_t, FILE *),
    T ptr, size_t size, size_t nmemb, FILE * stream)
{
    H3R_ENSURE(nmemb > 0 && size > 0, "r/w 0 bytes is forbidden")
    H3R_ENSURE(nullptr != ptr, "r/w via nullptr is forbidden")
    H3R_ENSURE(nullptr != stream, "r/w with null stream is forbidden")

    var sentinel = Ftell (stream);
    for (int i = 0; i < REASONABLE_RETRIES; i++) {
        if (   sentinel != H3R_FILE_OP_NOT_SUPPORTED
            && sentinel != Ftell (stream)) Fseek (stream, sentinel, SEEK_SET);
        //TODO does retry has meaning when H3R_FILE_OP_NOT_SUPPORTED?
        const var r = p (ptr, size, nmemb, stream);
        if (r == nmemb) return;
        const var e = H3R_OS_GLOBAL_ERR_CODE;
        H3R_ENSURE(0 != e, "POSIX r/w expected")
        ClearErr (stream);
        if (EAGAIN == e) Log_stderr ("//TODO: file r/w: handle EAGAIN" EOL);
        else if (EINTR == e) Log_stderr ("//TODO: file r/w: handle EINTR" EOL);
        // https://pubs.opengroup.org/onlinepubs/9699919799/functions/fread.html
        if (! RetryRead (e)) break;
    }
    H3R_ENSURE(false, "Unrecoverable file r/w error")
}

void Fread(void * ptr, size_t size, size_t nmemb, FILE * stream)
{
    RW (fread, ptr, size, nmemb, stream);
}

void Fwrite(const void * ptr, size_t size, size_t nmemb, FILE * stream)
{
    RW (fwrite, ptr, size, nmemb, stream);
}

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fopen.html#
FILE * Fopen(const char * path, const char * mode)
{
    H3R_ENSURE(nullptr != path, "path can't be null")
    H3R_ENSURE(nullptr != mode, "mode can't be null")

    for (int i = 0; i < REASONABLE_RETRIES; i++) {
        FILE * r = fopen (path, mode);
        if (r) return r;
        const var e = H3R_OS_GLOBAL_ERR_CODE;
        H3R_ENSURE(0 != e, "POSIX fopen expected")
        if (EAGAIN == e) Log_stderr ("//TODO: fopen: handle EAGAIN" EOL);
        else if (EINTR == e) Log_stderr ("//TODO: fopen: handle EINTR" EOL);
        // The POSIX explanation makes no sense: why would it need write access
        // to fopen() a directory?! Whats the point to fopen a directory?
        else if (EISDIR == e) Log_stderr ("//TODO: fopen: handle EISDIR" EOL);

        // Either a bug, or the user has to reconfigure the OS.
        else if (EMFILE == e) Log_stderr ("Process file limit overflow" EOL);
        else if (ENFILE == e) Log_stderr ("OS file limit overflow" EOL);

        else if (ENOENT == e || ENOTDIR == e)
            Log_stderr ("File not found?! : \"%s\"" EOL, path);
        else if (EROFS == e) Log_stderr ("RO file system" EOL);
        else if (EINVAL == e) Log_stderr ("Unknown \"mode\": \"\"" EOL, mode);

        if (! RetryOpen (e)) break;
    }
    H3R_ENSURE(false, "Unrecoverable fopen error")
}

void Fclose(FILE * stream)
{
    H3R_ENSURE(nullptr != stream, "can't close a null stream")
    H3R_ENSURE(stdout != stream, "closing stdout is a no no")
    H3R_ENSURE(stderr != stream, "closing stderr is a no no")
    H3R_ENSURE(stdin != stream, "closing stdin is a no no")

    for (int i = 0; i < REASONABLE_RETRIES; i++) {
        const var r = fclose (stream);
        if (0 == r) return;
        const var e = H3R_OS_GLOBAL_ERR_CODE;
        H3R_ENSURE(0 != e, "POSIX fclose expected")
        if (EAGAIN == e) Log_stderr ("//TODO: fclose: handle EAGAIN" EOL);
        else if (EINTR == e) Log_stderr ("//TODO: fclose: handle EINTR" EOL);
        else if (EBADF == e) Log_stderr ("fclose: invalid file descrptor" EOL);

        if (! RetryClose (e)) break;
    }
    H3R_ENSURE(false, "Unrecoverable fclose error")
}

bool FileExists(const char * path)
{
    struct stat t {};
    const var error = stat (path, &t);
    if (0 == error) return true;
    const var e = H3R_OS_GLOBAL_ERR_CODE;
    if (ENOENT == e) return false;
    switch (e) {
        case EACCES : Log_stderr ("stat - access denied: ") ; break;
        case EIO : Log_stderr ("stat - IO error: ") ; break;
        case ELOOP: Log_stderr ("stat - symlink loop: ") ; break;
        case ENAMETOOLONG: Log_stderr ("stat - name too long: ") ; break;
        default: Log_stderr ("stat - unhandled err: %d, ", e);
    };
    Log_stderr ("\"%s\"" EOL, path);
    H3R_ENSURE(false, "Unercoverable error during stat()")
}

off_t FileSize(const char * path)
{
    struct stat t {};
    for (int i = 0; i < REASONABLE_RETRIES; i++) {
        const var error = stat (path, &t);
        if (0 == error) return t.st_size;
        const var e = H3R_OS_GLOBAL_ERR_CODE;
        switch (e) {
            case ENOENT: Log_stderr ("stat - file not found: " EOL); break;
            case EACCES : Log_stderr ("stat - access denied: ") ; break;
            case EIO : Log_stderr ("stat - IO error: ") ; break;
            case ELOOP: Log_stderr ("stat - symlink loop: ") ; break;
            case ENAMETOOLONG: Log_stderr ("stat - name too long: ") ; break;
            default: Log_stderr ("stat - unhandled err: %d, ", e);
        };
        Log_stderr ("\"%s\"" EOL, path);
        if (! RetryOpen (e)) break;
    }
    H3R_ENSURE(false, "Unercoverable error during stat()")
}

} // namespace OS
NAMESPACE_H3R