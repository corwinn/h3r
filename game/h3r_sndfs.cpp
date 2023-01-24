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

#include "h3r_sndfs.h"
#include "h3r_log.h"

H3R_NAMESPACE

static int const H3R_SND_MAX_ENTRIES {1<<12};

SndFS::SndFS(const String & fname)
    : VFS {fname}
{
    H3R_CREATE_OBJECT(_s, OS::FileStream)
        {fname, H3R_NS::OS::FileStream::Mode::ReadOnly};
    if (! *_s) return;

    int cnt {0};

    Stream::Read (*_s, &cnt);
    if (cnt <= 0 || cnt > H3R_SND_MAX_ENTRIES) {
        Log::Err (String::Format (
            "%s: Suspicious entry count: %d" EOL, fname.AsZStr (), cnt));
        return;
    }
    OS::Log_stdout ("%s: entries: %d" EOL, fname.AsZStr (), cnt);

    _entries.Resize (cnt);
    var data = static_cast<SndFS::Entry *>(_entries);
    Stream::Read (*_s, data, cnt);
    // Validate
    var file_size = _s->Size ();
    for (size_t i = 0; i < _entries.Length (); i++) {
        var & e = _entries[i];
        if (e.Ofs < _s->Tell () || e.Ofs >= file_size) {
            Log::Err (String::Format ("%s: Wrong Entry[%0004d].Ofs: "
                "%zu, out of [%00000008d;%00000008d)" EOL,
                fname.AsZStr (), i, e.Ofs, _s->Tell (), file_size));
            return;
        }
        int b = i < _entries.Length ()-1 ? _entries[i+1].Ofs : file_size;
        if ((b - e.Ofs) != e.Size) {
            Log::Err (String::Format ("%s: Wrong Entry[%0004d].Size: Actual: "
                "%zu, Expected: %zu" EOL, fname.AsZStr (), i, e.Size, b));
            return;
        }
        e.Name[39] = '\0';
        // There is an odd \0 as file extension separator
        for (int k = 0; k < 40; k++)
            if ('\0' == e.Name[k]) {
                e.Name[k] = '.';
                // The "gog" version adds more code here
                if (k+4 < 40) e.Name[k+4] = '\0';
                break;
        }
    }
    /*int j {0};
    for (const var & e : _entries)
        OS::Log_stdout (
            "%s: entry: %004d: %00000008d:%00000008d \"%s\"" EOL,
            fname.AsZStr (), j++, e.Ofs, e.Size, e.Name);*/

    H3R_CREATE_OBJECT(_rrs, RefReadStream) {_s, 0, 0};
    _usable = true;
}// SndFS::SndFS()

SndFS::~SndFS()
{
    H3R_DESTROY_OBJECT(_rrs, RefReadStream)
    H3R_DESTROY_OBJECT(_s, FileStream)
}

Stream & SndFS::GetStream(const SndFS::Entry & e)
{
//np start: e.Ofs, size: e.Size
    return _rrs->ResetTo (e.Ofs, e.Size);
}

Stream * SndFS::Get(const String & res)
{
    for (const var & e : _entries)
        if (res == reinterpret_cast<const char *>(e.Name))
            return &(GetStream (e));
    return VFS::Get (res);
}
//TODO Notify - see LodFS::Walk
void SndFS::Walk(bool (*on_entry)(Stream &, const VFS::Entry &))
{
    static VFS::Entry vfs_e {};
    for (const var & e : _entries) {
        vfs_e.Name = reinterpret_cast<const char *>(e.Name);
        vfs_e.Size = e.Size;
        if (! on_entry (GetStream (e), vfs_e)) break;
    }
}

NAMESPACE_H3R