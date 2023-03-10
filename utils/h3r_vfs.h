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

#ifndef _H3R_VFS_H_
#define _H3R_VFS_H_

// virtual file system - abstract away the game files

#define VERY_IMPORTANT_AND_VERY_USELESS_MESSAGE "Resource system initialized()"

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_stream.h"
#include "h3r_taskstate.h"

H3R_NAMESPACE

//TODO unicode support
// Consider ASCII to be the safe choice for your filenames.
class VFS
{
    H3R_CANT_COPY(VFS)
    H3R_CANT_MOVE(VFS)

    public enum class FileType {nvm,
        bik, def, fnt, h3c, h3m, h3r, mp3, msk, pal, pcx, smk, txt, wav};

    // Reflection-only constructor - for TryLoad()
    public VFS() {}

    public VFS(const String & path);
    public virtual ~VFS();

    // You request by name - you get a stream reference. You request the same
    // name again - you could get another stream, or the same stream but in a
    // reset state (pos=0, length could be different).
    // For an archive file (.lod, .snd, .vid) - you get a RefReadStream;
    // releasing it, is not your business. Just be careful to not access it
    // after the VFS has been released.
    // You shall have access to a limited number of distinct streams, decided by
    // each VFS; for example the FS one shall limit the number in accordance
    // with the OS file handle limitations, etc.
    // use-case 1: on_game_start (cache all needed resources into RAM).
    // use-case 2: on_map_load (cache all needed resources into RAM).
    // use-case 3: on_map_load (invalidate, and do use-case #2)
    //
    // Use AsyncAdapter with the stream.
    //TODO AsyncAdapter & Get()? or Game::AsyncStreamAdapter?
    public virtual Stream * Get(const String & name);

    // When the VFS is ok for use; indicates errors during constructor calls.
    public virtual operator bool() const;

    // There is no need for VFSWalker (3rd party actor (called Strategy):
    // a polymorphic class that you extend with concrete walkers as needed; this
    // becomes the Context in that use-case) for this project.
    // Should be enough: Name, Size, and Date/Time later probably
    public class Entry
    {
        public String Name;
        public off_t Size;
    };

    // Observer: return false to interrupt the walk
    public virtual void Walk(bool (*)(Stream &, const VFS::Entry &));

    //TODO Design me

    // Create new instance
    public virtual VFS * TryLoad(const String &) { return nullptr; }

#undef public
    public: class VFSInfo final : public TaskState
#define public public:
    {
        private bool _pp {};
        public void SetPercentageProgress(bool value) { _pp = value; }
        public VFSInfo() : TaskState {0, ""} {}
        public using TaskState::TaskState;
        public inline bool PercentageProgress() const override { return _pp; }
    };

    public class VFSEvent
    {
        private VFSEvent * _next;
        public virtual void Do(VFSInfo * a) { if (_next) _next->Do (a); }
        public VFSEvent(VFSEvent * next_eh = nullptr) : _next{next_eh} {}
        public virtual void SetNext(VFSEvent * next_eh) { _next = next_eh; }
    };

    // Use-case-1; OnProgress.SetNext(your_event_handler *)
    //PERHAPS If the above fails (is not enough; you want to show five progress
    //        bars simultaneously for example); Use-case-2:
    //        public VFSEvent * OnProgress; // Whatever you set that pointer to,
    //                                      // shall be OnProgress->Do (nullptr)
    // Remember! All IO is done in a thread != main, so this event won't be your
    // favorite thread.
    //TODO think of something to clearly mark such functions.
    public VFSEvent OnProgress;
};// VFS

NAMESPACE_H3R

#endif