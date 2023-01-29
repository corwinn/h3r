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

#include "h3r_lodfs.h"
#include "h3r_log.h"

H3R_NAMESPACE

static int const H3R_LOD_SIGN {0x444f4c};
static int const H3R_LOD_MAX_ENTRIES {1<<13};
static int const H3R_LOD_UNK1 {4}; // unknown 4 bytes
static int const H3R_LOD_UNK2 {80};// unknown 80 bytes

LodFS::LodFS(const String & fname)
    : VFS {fname}
{
    H3R_CREATE_OBJECT(_s, OS::FileStream)
        {fname, H3R_NS::OS::FileStream::Mode::ReadOnly};
    if (! *_s) return;

    union { int isign; unsigned char sign[4]; };
    int cnt {0};

    Stream::Read (*_s, &isign);
    if (H3R_LOD_SIGN != isign) {
        Log::Err (String::Format ("%s: Unknown signature: %00000008Xd" EOL,
            fname.AsZStr (), isign));
    }
    OS::Log_stdout ("%s: sign: %s" EOL, fname.AsZStr (), sign);

    _s->Seek (H3R_LOD_UNK1); //TODO what are those? - c8 @ H3bitmap.lod

    Stream::Read (*_s, &cnt);
    if (cnt <= 0 || cnt > H3R_LOD_MAX_ENTRIES) {
        Log::Err (
            String::Format (
                "%s: Suspicious entry count: %d" EOL, fname.AsZStr (), cnt));
        return;
    }
    OS::Log_stdout ("%s: entries: %d" EOL, fname.AsZStr (), cnt);

    _s->Seek (H3R_LOD_UNK2); //TODO what are those? H3bitmap.lod

    _entries.Resize (cnt);
    auto data = static_cast<LodFS::Entry *>(_entries);
    Stream::Read (*_s, data, cnt);
    //TODO validate entries
    /*int i {0};
    for (const auto & e : _entries)
        OS::Log_stdout (
            "%s: entry: %004d: %c (%00000008d/%00000008d) \"%s\"" EOL,
            fname.AsZStr (), i++, (e.SizeU > e.SizeC && e.SizeC > 0 ? 'C' : 'U'),
                e.SizeC, e.SizeU, e.Name);*/
    //LATER if (! sorted) sort (); contract with the binary search below;
    //      or hash (you should have a DLL already)

    H3R_CREATE_OBJECT(_rrs, RefReadStream) {_s, 0, 0};
    H3R_CREATE_OBJECT(_zis, ZipInflateStream) {_rrs, 0, 0};
    _usable = true;
}// LodFS::LodFS()

LodFS::~LodFS()
{
    H3R_DESTROY_OBJECT(_zis, ZipInflateStream)
    H3R_DESTROY_OBJECT(_rrs, RefReadStream)
    H3R_DESTROY_OBJECT(_s, FileStream)
}

Stream & LodFS::GetStream(const LodFS::Entry & e)
{
    // its compressed:
    // H3bitmap.lod: entry: 3862: U (00000074/00000074) "TERRNAME.txt"
    bool compressed = e.SizeU >= e.SizeC && e.SizeC > 0;
    /*OS::Log_stdout ("LodFS::GetStream: compressed: %s; %s" EOL,
        (compressed ? "true" : "false"), e.Name);*/
//np start: e.Ofs, size: (compressed ? e.SizeC : e.SizeU)
    _rrs->ResetTo (e.Ofs, (compressed ? e.SizeC : e.SizeU));
    return compressed
//np size: e.SizeC, usize: e.SizeU
        ? _zis->ResetTo (e.SizeC, e.SizeU)
//np start: e.Ofs, size: e.SizeU
        : _rrs->ResetTo (e.Ofs, e.SizeU);
}

Stream * LodFS::Get(const String & res)
{
    //LATER binary search; sort the entry list (I think they're sorted already);
    //      What?! - you expected a hash? - there will be one sooner or later,
    //      no worries.
    for (const auto & e : _entries)
        if (res == reinterpret_cast<const char *>(e.Name))
            return &(GetStream (e));
    return VFS::Get (res);
}

void LodFS::Walk(bool (*on_entry)(Stream &, const VFS::Entry &))
{
    static VFS::Entry vfs_e {};
    auto prev_info = VFS::VFSInfo {0, ""};
    auto all = _entries.Length ();
    auto i = all-all;
    for (const auto & e : _entries) {
        vfs_e.Name = reinterpret_cast<const char *>(e.Name);
        vfs_e.Size = e.SizeU;
        if (! on_entry (GetStream (e), vfs_e)) break;

        //TODO there is definitely a field for improvements, still
        auto new_info = VFS::VFSInfo {
            static_cast<int>(0.5+(1.0*i++/all*100)), (String &&)vfs_e.Name};
        if (TaskState::Changed (prev_info, new_info)) {
            new_info.SetChanged (true);
            OnProgress.Do (&new_info);
            prev_info = (VFS::VFSInfo &&)new_info;
        }
    }
}

NAMESPACE_H3R