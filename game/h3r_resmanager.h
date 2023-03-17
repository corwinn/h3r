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

#ifndef _H3R_RESMANAGER_H_
#define _H3R_RESMANAGER_H_

#include "h3r.h"
#include "h3r_vfs.h"
#include "h3r_array.h"
#include "h3r_iasynctask.h"
#include "h3r_list.h"
#include "h3r_criticalsection.h"
#include "h3r_timing.h"

#undef public
#undef private
#undef protected
#include <cmath>
// Undef for the "extends" lines only
#define public public:
#define private private:
#define protected protected:

H3R_NAMESPACE

// The Grand FS.
// It is not final because it can be overridden with a plug-in.
//TODO FVS -> IVFS
//TODO Design me:
//      * Clarify all actors. Clarify them once more.
//      * Prove there are no deadlocks.
//      * Prove there are no race conditions.
//      * Prove the above two, nine more times each.
//      * ResManager is everything that needs Async IO.
//      * Advantage(s) of IOThread being at Game, besides init?
//      * Remember the plug-in aspect.
//LATER Register(ResManager)? pros and cons; right now doing so is a bad idea
#undef protected
class ResManager : protected VFS
#define protected protected:
{
    H3R_CANT_COPY(ResManager)
    H3R_CANT_MOVE(ResManager)

    // Hide the VFS constructor because the ResManager doesn't deal with paths;
    // it deals with VFS.
    //LATER make it private if no use-case presents itself; e.g. no plug-in
    //      requires it
    protected ResManager(const String & path)
        : VFS {path}, _load_task {*this}, _walk_task {*this},
            _get_task {*this}, _on_progress_delegate {&OnProgress} {}
    public ResManager()
        : VFS {}, _load_task {*this}, _walk_task {*this}, _get_task {*this},
            _on_progress_delegate {&OnProgress} {}
    public ~ResManager() override;
    private static OS::CriticalSection _task_info_gate;

#undef public
    public: class RMTaskInfo final : public TaskState
#define public public:
    {
        public using TaskState::TaskState;
        // Shall be ok until you request a new one. One of the reasons for One
        // task at time rule (see Game::IOThread); or I would be thinking about
        // stream copying - and I don't want to.
        public Stream * Resource {};
        public bool (*WalkCallback)(Stream &, const VFS::Entry &) {};
        public String Path {};
        public String Name {};
        public bool Result {};

        //TODO should this go to TaskState itself, or someplace else?
        //TODO Design me
        private TaskState _progress {0, ""};
        public TaskState GetInfo() //TODO ensure this returns a copy
        {
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ____ {_task_info_gate};
            auto result = _progress; _progress.SetChanged (false);
            return result;
        }
        public void SetInfo(const TaskState & value)
        {
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ____ {_task_info_gate};
            if (TaskState::Changed (_progress, value))
                (_progress = value).SetChanged (true);
        }
    };

#undef public
    private class RMTask : public IAsyncTask
#define public public:
    {
        protected ResManager & _subject;
        public RMTaskInfo State;
        public inline TaskState Whatsup() override { return State; }
        public RMTask(ResManager & subject)
            : _subject{subject}, State {0, ""} {}
        public IAsyncTask & SetPath(const String & path)
        {
            return State.Path = path, *this;
        }
        public IAsyncTask & SetName(const String & name)
        {
            return State.Name = name, *this;
        }
    };

#undef public
    private class RMLoadTask final : public RMTask
#define public public:
    {
        public using RMTask::RMTask;
        public inline void Do() override
        {//TODO progress; see RMGetTask
            State.Result = false;
            State.SetInfo (TaskState {0, "Loading: " + State.Path});
            for (auto * vfs : _subject._vfs_registry)
                State.Result |= _subject.AddVFS (vfs->TryLoad (State.Path));
            State.SetInfo (TaskState {0, "Loaded: " + State.Path});
        }
    } _load_task;

#undef public
    private class RMWalkTask final : public RMTask
#define public public:
    {
        public using RMTask::RMTask;
        public inline void Do() override
        {//TODO progress; see RMGetTask
            auto all = _subject._vfs_objects.Count ();
            auto i = all - all;
            for (auto * vfs : _subject._vfs_objects) {
                State.SetInfo (TaskState {
                    static_cast<int>(round (1.0*i/all*100)),
                    "Enumerating ..."});
                vfs->Walk (State.WalkCallback);
                State.SetInfo (TaskState {
                    static_cast<int>(round (1.0*++i/all*100)),
                    "Enumerated"});
            }
        }
        public IAsyncTask & SetCallback(bool (*c)(Stream &, const VFS::Entry &))
        {
            return State.WalkCallback = c, *this;
        }
    } _walk_task;

#undef public
    private class RMGetTask final : public RMTask
#define public public:
    {
        public using RMTask::RMTask;
        public inline void Do() override
        {
            /*static OS::TimeSpec time_a, time_b;
            OS::GetCurrentTime (time_a);*/

            State.Resource = nullptr;
            // auto all = _subject._vfs_objects.Count ();
            // auto i = all - all;
            for (auto * vfs : _subject._vfs_objects) {
                // State.SetInfo (TaskState {
                //    static_cast<int>(round (1.0*i++/all*100)),
                //    "Looking for: " + State.Name});
                // Its assignment, not comparison.
                if (nullptr != (State.Resource = vfs->Get (State.Name))) break;
            }

            /*OS::GetCurrentTime (time_b);
            auto frame_time = OS::TimeSpecDiff (time_a, time_b); // [nsec]
            printf ("RMGetTask: %lu [nsec]" EOL, frame_time);*/
        }
        public Stream * GetStream()
        {
            return State.Resource;
        }
    } _get_task;

    // AsyncIO: Return the 1st matching one.
    //TODO what about duplicates? res override policy
    // Usage:
    //   auto res = RM.GetResource ("foo");
    //   while (! RM.Complete ())
    //       UpdateProgressBar (res.GetInfo ());
    //   auto stream = res.Stream;
    //   use AsyncAdapter to read from the stream;
    public virtual const RMTaskInfo & GetResource(const String & name);

    public virtual inline operator bool() const override
    {
        return ! _vfs_registry.Empty ();
    }

    // AsyncIO: Enumerate all resources. Usage: See GetResource
    // Observer: return false to interrupt the walk
    public virtual const RMTaskInfo & Enumerate(
        bool (*)(Stream &, const VFS::Entry &));

    // This is a collection of VFS, not a VFS handler.
    private inline VFS * TryLoad(const String &) override { return nullptr; }

    private List<VFS *> _vfs_registry {};
    private List<VFS *> _vfs_objects {};
    public inline virtual void Register(VFS * vfs)
    {
        H3R_ARG_EXC_IF(_vfs_registry.Contains (vfs), "Consider this Map<Type>")
        _vfs_registry.Add (vfs);
    }

#undef public
    private class EventFwd final : public VFSEvent
#define public public:
    {
        public using VFSEvent::VFSEvent;
        // Looks like an Observer; "e" shall notify "this".
        public void SubscribeTo(VFSEvent * e) { e->SetNext (this); }
        public void Do(VFS::VFSInfo * info) override { VFSEvent::Do (info); }
    } _on_progress_delegate;

    // There could be VFS that have their own threading, like the FS one using
    // AsyncFsEnum. How do this thread deals with them: waits for them to
    // complete their init.: "Waiting for foo to complete, please wait..."
    private bool AddVFS(VFS * obj)
    {
        if (obj) {
            // forward VFS events to "this"
            _on_progress_delegate.SubscribeTo (&(obj->OnProgress));
            _vfs_objects.Add (obj);
            return true;
        }
        else return false;
    }

    // AsyncIO: Load a VFS that can handle "path". Usage: See GetResource
    //TODO Block duplicate path
    public virtual const RMTaskInfo & Load(const String & path);

    // The base RM does its work in Game::IOThread. Its recommended that any
    // plug-in shall use its own thread.
    public virtual bool TaskComplete();
};// ResManager

NAMESPACE_H3R

#endif