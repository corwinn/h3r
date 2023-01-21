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
    : VFS {fname}, _s {fname, H3R_NS::OS::FileStream::Mode::ReadOnly},
        _rrs {&_s, 0, 0}, _zis {&_rrs, 0, 0}
{
    union { int isign; unsigned char sign[4]; };
    int cnt {0};
    var FN_cleanup {fname.AsZStr ()};
    var FN {FN_cleanup.Data ()};

    Stream::Read (_s, &isign);
    if (H3R_LOD_SIGN != isign) {
        Log::Err (String::Format ("%s: Unknown signature: %00000008Xd" EOL,
            FN, isign));
    }
    OS::Log_stdout ("%s: sign: %s" EOL, FN, sign);

    _s.Seek (H3R_LOD_UNK1); //TODO what are those? - c8 @ H3bitmap.lod

    Stream::Read (_s, &cnt);
    if (cnt <= 0 || cnt > H3R_LOD_MAX_ENTRIES) {
        Log::Err (
            String::Format ("%s: Suspicious entry count: %d" EOL, FN, cnt));
        return;
    }
    OS::Log_stdout ("%s: entries: %d" EOL, FN, cnt);

    _s.Seek (H3R_LOD_UNK2); //TODO what are those? H3bitmap.lod

    _entries.Resize (cnt);
    var data = static_cast<Entry *>(_entries);
    Stream::Read (_s, data, cnt);
    /*int i {0};
    for (const var & e : _entries)
        OS::Log_stdout (
            "%s: entry: %004d: %c (%00000008d/%00000008d) \"%s\"" EOL,
            FN, i++, (e.SizeU > e.SizeC && e.SizeC > 0 ? 'C' : 'U'),
                e.SizeC, e.SizeU, e.Name);*/

    _usable = true;
}// LodFS::LodFS()

LodFS::~LodFS() {}

Stream & LodFS::GetStream(const LodFS::Entry & e)
{
    // its compressed:
    // H3bitmap.lod: entry: 3862: U (00000074/00000074) "TERRNAME.txt"
    bool compressed = e.SizeU >= e.SizeC && e.SizeC > 0;
    /*OS::Log_stdout ("LodFS::GetStream: compressed: %s" EOL,
        (compressed ? "true" : "false"));*/
    _rrs.ResetTo (e.Ofs, (compressed ? e.SizeC : e.SizeU));
    return compressed
//np start: e.Ofs, size: compressed ? e.SizeC : e.SizeU
        ? _zis.ResetTo (e.SizeC, e.SizeU)
//np pos: 0, size: e.SizeC, usize: e.SizeU
        : _rrs.ResetTo (e.Ofs, e.SizeU);
}

Stream & LodFS::Get(const String & res)//TODO come back after unifying the entry
{
    //TODO binary search; sort the entry list (I think they're sorted already)
    //     What?! - you expected a hash? - there will be one sooner or later,
    //     no worries.
    for (const var & e : _entries)
        if (res.EqualsZStr ((const char *)e.Name)) return GetStream (e);
    return VFS::Get (res);
}

void LodFS::Walk(bool (*on_entry)(Stream &, const char * name))
{
    for (var & e : _entries)
        if (! on_entry (GetStream (e), (const char *)e.Name)) break;
}

NAMESPACE_H3R