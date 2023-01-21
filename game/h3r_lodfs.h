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

H3R_NAMESPACE

class LodFS: public VFS
{
    private: OS::FileStream _s;
    private: bool _usable {false};
    // 1 stream for now
    private: RefReadStream _rrs;
    private: ZipInflateStream _zis;
#pragma pack(push, 1)
    private: struct Entry final
    {
        unsigned char Name[16];
        int Ofs;   // SEEK_SET
        int SizeU; // Uncompressed size [bytes]
        int Type;
        int SizeC; // Compressed size [bytes]
    };
#pragma pack(pop)
    private: Array<Entry> _entries {};
    private: Stream & GetStream(const LodFS::Entry &);
    public: LodFS(const String & path);
    public: ~LodFS() override;
    public: virtual Stream & Get(const String & name) override;
    public: virtual inline operator bool() const override { return _usable; }

    public: void Walk(bool (*on_entry)(Stream &, const char * name));
};

NAMESPACE_H3R

#endif