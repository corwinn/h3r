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

#ifndef _H3R_ASYNCFSENUM_H_
#define _H3R_ASYNCFSENUM_H_

#include "h3r.h"
#include "h3r_os.h"
#include "h3r_string.h"
#include "h3r_stack.h"
#include "h3r_iasynctask.h"
#include "h3r_taskstate.h"
#include "h3r_taskthread.h"

H3R_NAMESPACE

// Recursive (directory) asynchronous FS enumerator.
// T - observer;
// bool (T::*)(const EnumItem &) - event handler called in async thread context;
// EnumItem a.k.a. EventArgs:
//   Name        - the full path name (starting with base_path)
//   FileName    - just the file name
//   IsDirectory - whether this is directory
//   Progress    - the sequential number of the found file
//   PercentageProgress () - false, because I have yet to find an FS that does
//                           support file num per directory tree
template <typename T> class AsyncFsEnum final
{
#undef public
    public: class EnumItem final : public TaskState
#define public public:
    {
        using TaskState::TaskState;
        public inline virtual bool PercentageProgress() const { return false; }
        public String Name {};
        public String FileName {};
        public bool IsDirectory {};
        public int Progress {};
    };
    // since the notification is in the task thread context
    using OnItem = bool (T::*)(const EnumItem &);
    using OnComplete = void (T::*)();
    private T * _on_item_observer;
    private OnItem _on_item;
    private OnComplete _done;
    private bool ThrNotify()
    {
        return (_on_item_observer->*_on_item) (_task.State);
    }
    private void ThrNotifyComplete()
    {
        (_on_item_observer->*_done) ();
    }
    private String _root;
    // It requires its own thread to load other file IO while enumerating.
    private TaskThread _thread {};
    public AsyncFsEnum(String base_path, T * observer,
        OnItem handle_on_item, OnComplete handle_on_complete)
        : _on_item_observer{observer}, _on_item{handle_on_item},
        _done{handle_on_complete}, _root{base_path}, _task {*this}
    {
        _thread.Task = _task;
    }

#undef public
    private class EnumTask final : public IAsyncTask
#define public public:
    {
        private AsyncFsEnum<T> & _subject;
        private Stack<String> _directories {};
        private String _current_root {};
        private int _stack_load {};
        public EnumItem State;
        public EnumTask(AsyncFsEnum<T> & subject)
            : _subject{subject}, State {0, "Enumerating..."} {}
        public inline TaskState Whatsup() override { return State; }
        public inline void Do() override
        {
            _directories.Push (_subject._root); _stack_load++;
            while (! _directories.Empty ())
                H3R_NS::OS::EnumFiles<EnumTask> (*this,
                    _current_root = static_cast<String &&>(_directories.Pop ()),
                    [](EnumTask & task, const char * name, bool dir) -> bool
                    {
                        task.State.FileName = name;
                        task.State.Name = static_cast<String &&>(
                            String::Format ("%s%c%s", task._current_root
                                .AsZStr (), H3R_PATH_SEPARATOR, name));
                        task.State.IsDirectory = dir;
                        task.State.Progress++;
                        if (task.State.IsDirectory)
                            task._directories.Push (task.State.Name),
                            task._stack_load =
                                task._directories.Size () > task._stack_load ?
                                task._directories.Size () : task._stack_load;
                    return task._subject.ThrNotify ();
                });
            _subject.ThrNotifyComplete ();
            H3R_NS::OS::Log_stdout ("stack_load: %zu" EOL, _stack_load);
        }
    } _task;
    public bool Complete() const { return _thread.Done (); }
};// AsyncFsEnum

NAMESPACE_H3R
#endif
