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

#include "h3r_vidfs.h"
#include "h3r_log.h"

H3R_NAMESPACE

static int const H3R_VID_MAX_ENTRIES {1<<10};

VidFS::VidFS(const String & fname)
    : VFS {fname}
{
    H3R_CREATE_OBJECT(_s, OS::FileStream)
        {fname, H3R_NS::OS::FileStream::Mode::ReadOnly};
    if (! *_s) return;
    _last_offset = {_s->Size ()};

    int cnt {0};

    Stream::Read (*_s, &cnt);
    if (cnt <= 0 || cnt > H3R_VID_MAX_ENTRIES) {
        Log::Err (String::Format (
            "%s: Suspicious entry count: %d" EOL, fname.AsZStr (), cnt));
        return;
    }
    OS::Log_stdout ("%s: entries: %d" EOL, fname.AsZStr (), cnt);

    _entries.Resize (cnt);
    auto data = static_cast<VidFS::Entry *>(_entries);
    Stream::Read (*_s, data, cnt);
    // Validate
    for (size_t i = 0; i < _entries.Length (); i++) {
        int a = _entries[i].Ofs;
        int b = i < _entries.Length ()-1 ? _entries[i+1].Ofs : _last_offset;
        if (b <= a) {
            Log::Err (String::Format ("%s: Wrong Entry[%003d].Ofs: "
                "Entry%003d.Ofs: %zu >= Entry%003d.Ofs: %zu" EOL,
                fname.AsZStr (), i, i, a, i+1, b));
            return;
        }
        _entries[i].Name[39] = '\0';
    }
    /*int j {0};
    for (const auto & e : _entries)
        OS::Log_stdout (
            "%s: entry: %004d: %00000008d \"%s\"" EOL,
            fname.AsZStr (), j++, e.Ofs, e.Name);*/

    H3R_CREATE_OBJECT(_rrs, RefReadStream) {_s, 0, 0};
    _usable = true;
}// VidFS::VidFS()

VidFS::~VidFS()
{
    H3R_DESTROY_OBJECT(_rrs, RefReadStream)
    H3R_DESTROY_OBJECT(_s, FileStream)
}

Stream & VidFS::GetStream(const VidFS::Entry & e, int size)
{
//np start: e.Ofs, size: size
    return _rrs->ResetTo (e.Ofs, size);
}

Stream * VidFS::Get(const String & res)
{
    for (size_t i = 0; i < _entries.Length (); i++)
        if (res == reinterpret_cast<const char *>(_entries[i].Name))
            return &(GetStream (_entries[i], GetSize (i)));
    return VFS::Get (res);
}

int VidFS::GetSize(size_t idx)
{
    auto size = (idx < _entries.Length ()-1 ? _entries[idx+1].Ofs : _last_offset)
        - _entries[idx].Ofs;
    H3R_ENSURE(size > 0, "Validate again")
    return size;
}
//TODO Notify - see LodFS::Walk
void VidFS::Walk(bool (*on_entry)(Stream &, const VFS::Entry &))
{
    static VFS::Entry vfs_e {};
    for (size_t i = 0; i < _entries.Length (); i++) {
        vfs_e.Name = reinterpret_cast<const char *>(_entries[i].Name);
        vfs_e.Size = GetSize (i);
        if (! on_entry (GetStream (_entries[i], vfs_e.Size), vfs_e)) break;
    }
}

NAMESPACE_H3R