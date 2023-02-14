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

#ifndef _H3R_LODFS_H_
#define _H3R_LODFS_H_

#include "h3r.h"
#include "h3r_vfs.h"
#include "h3r_string.h"
#include "h3r_stream.h"
#include "h3r_filestream.h"
#include "h3r_array.h"
#include "h3r_refreadstream.h"
#include "h3r_zipinflatestream.h"
#include "h3r_memorystream.h"
#include "h3r_resnamehash.h"

// placement new
#include <new>

#define IMPROVISED_CACHE

H3R_NAMESPACE

// Get() will return the same pointer on each invocation! Means: each Get()
// invalidates the contents of the previous one, unless you requested the same
// resource with the same content. If you need your data for later use, you
// better copy it, because the next call to Get() could invalidate it.
//
// On 2nd thought this shouldn't be final, should one decide to use own,
// extended format (better compression for one, and or hashed names, etc.).
class LodFS : public VFS
{
#define public public:
#define private private:
#define protected protected:

#ifdef IMPROVISED_CACHE
    // Improvised cache
    private struct CacheEntry final
    {
        Stream * S {};
        Stream * Sd {}; // decorator stream ptr: cleanup
        int Count {1};
    };
    private int const _CACHE_SIZE {1<<23}; // [bytes]
    private int _cache_size {0}; // [bytes]
    private ResNameHash<LodFS::CacheEntry> _cache {};
#endif

    protected OS::FileStream * _s {};
    protected bool _usable {false};
    // 1 stream for now
    protected RefReadStream * _rrs {};
    protected ZipInflateStream * _zis {};
#pragma pack(push, 1)
    protected struct Entry final
    {
        unsigned char Name[16];
        int Ofs;   // SEEK_SET
        int SizeU; // Uncompressed size [bytes]
        int Type;
        int SizeC; // Compressed size [bytes]
        inline bool Compressed() { return SizeU >= SizeC && SizeC > 0; }
    };
#pragma pack(pop)
    protected ResNameHash<LodFS::Entry> _entries {};
    protected virtual Stream & GetStream(const LodFS::Entry &);
    public LodFS(const String & path);
    public ~LodFS() override;
    public virtual Stream * Get(const String & name) override;
    public virtual inline operator bool() const override { return _usable; }

    public virtual void Walk(bool (*)(Stream &, const VFS::Entry &)) override;

    public LodFS() : VFS {} {}
    public inline virtual VFS * TryLoad(const String & path) override
    {
        if (! path.ToLower ().EndsWith (".lod")) return nullptr;
        LodFS * result {};
        H3R_CREATE_OBJECT(result, LodFS) {path};
        if (*result) return result;
        H3R_DESTROY_OBJECT(result, LodFS)
        return nullptr;
    }

#undef public
#undef private
#undef protected
};// LodFS

NAMESPACE_H3R

#endif