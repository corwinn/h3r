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

#include "h3r_resmanager.h"
#include "h3r_game.h"

H3R_NAMESPACE

OS::CriticalSection ResManager::_task_info_gate {};

ResManager::~ResManager()//TODO shouldn't I be an IAsyncTask?
{
    for (var * obj : _vfs_objects) H3R_DESTROY_OBJECT(obj, VFS)
    for (var * obj : _vfs_registry) H3R_DESTROY_OBJECT(obj, VFS)
}

const ResManager::RMTaskInfo & ResManager::GetResource(const String & name)
{
    Game::IOThread.Task = _get_task.SetName (name);
    return _get_task.State;
}

const ResManager::RMTaskInfo & ResManager::Enumerate(
    bool (*on_entry)(Stream &, const VFS::Entry &))
{
    Game::IOThread.Task = _walk_task.SetCallback (on_entry);
    return _walk_task.State;
}

const ResManager::RMTaskInfo & ResManager::Load(const String & path)
{
    Game::IOThread.Task = _load_task.SetPath (path);
    return _load_task.State;
}

bool ResManager::TaskComplete() { return Game::IOThread.Done (); }

NAMESPACE_H3R