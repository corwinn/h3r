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

    Array<LodFS::Entry> entries {cnt};
    auto data = static_cast<LodFS::Entry *>(entries);
    Stream::Read (*_s, data, cnt);
    for (auto & e : entries)
        _entries.Add (String {reinterpret_cast<const char *>(e.Name)}
            .operator const Array<byte> & (), e);
    //TODO validate entries
    /*int i {0};
    for (const auto & e : _entries)
        OS::Log_stdout (
            "%s: entry: %004d: %c (%00000008d/%00000008d) \"%s\"" EOL,
            fname.AsZStr (), i++, (e.SizeU > e.SizeC && e.SizeC > 0 ? 'C' : 'U'),
                e.SizeC, e.SizeU, e.Name);*/

    H3R_CREATE_OBJECT(_rrs, RefReadStream) {_s, 0, 0};
    H3R_CREATE_OBJECT(_zis, ZipInflateStream) {_rrs, 0, 0};
    _usable = true;
}// LodFS::LodFS()

LodFS::~LodFS()
{
    printf ("LodFS::~LodFS()" EOL);
    H3R_DESTROY_OBJECT(_zis, ZipInflateStream)
    H3R_DESTROY_OBJECT(_rrs, RefReadStream)
    H3R_DESTROY_OBJECT(_s, FileStream)

#ifdef IMPROVISED_CACHE
    for (auto & e : _cache) {
        if (e->Value.Count > 1) printf ("Cache entry.Count: %3d" EOL,
            e->Value.Count);
        if (e->Value.Sd) H3R_DESTROY_OBJECT(e->Value.Sd, Stream)
        if (e->Value.S) H3R_DESTROY_OBJECT(e->Value.S, Stream)
    }
#endif
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
#ifdef IMPROVISED_CACHE
    LodFS::CacheEntry cached_entry {};
    if (_cache.TryGetValue (res.operator const Array<byte> &(), cached_entry)) {
        printf ("Cached: %s" EOL, res.AsZStr ());
        return cached_entry.S;
    }
#endif
    LodFS::Entry e {};
    if (_entries.TryGetValue (res.operator const Array<byte> &(), e)) {
#ifdef IMPROVISED_CACHE
        int size = e.Compressed () ? e.SizeC : e.SizeU;
        if (size + _cache_size < _CACHE_SIZE) {
            printf ("Caching: %s, Compressed: %s, Ofs: %10d, Size: %7d, "
                "Uncompressed size: %7d, Type: %d" EOL, res.AsZStr (),
                (e.Compressed () ? "true" : "false"), e.Ofs, size, e.SizeU,
                e.Type);
            _cache_size += size;
            _s->Seek (e.Ofs - _s->Tell ());
            MemoryStream * ms {};
#ifdef IMPROVISED_CACHE_UNZIP
            if (e.Compressed ()) {
                // cache unpacked; ok its not it; see "async-ui-issue.dia"
                ZipInflateStream zis {_s, size, e.SizeU};
                H3R_CREATE_OBJECT(ms, MemoryStream) {&zis, e.SizeU};
            }
            else {
                H3R_CREATE_OBJECT(ms, MemoryStream) {_s, size};
            }
            cached_entry.S = ms;
#else
            H3R_CREATE_OBJECT(ms, MemoryStream) {_s, size}; // read it to RAM
            if (! e.Compressed ())
                cached_entry.S = ms;
            else {
                ZipInflateStream * zis {};
                H3R_CREATE_OBJECT(zis, ZipInflateStream) {ms, size, e.SizeU};
                cached_entry.S = zis; cached_entry.Sd = ms;
            }
#endif
            _cache.Add (res.operator const Array<byte> &(), cached_entry);
            return cached_entry.S;
        }
        else // cache full -> fallback to uncached, for now
#endif
            return &(GetStream (e));
    }
    // printf ("Not found: %s" EOL, res.AsZStr ());
    return VFS::Get (res);
}

void LodFS::Walk(bool (*on_entry)(Stream &, const VFS::Entry &))
{
    static VFS::Entry vfs_e {};
    auto prev_info = VFS::VFSInfo {0, ""};
    auto all = _entries.Count ();
    auto i = all-all;
    for (auto e : _entries) {
        vfs_e.Name = reinterpret_cast<const char *>(e->Value.Name);
        vfs_e.Size = e->Value.SizeU;
        if (! on_entry (GetStream (e->Value), vfs_e)) break;

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