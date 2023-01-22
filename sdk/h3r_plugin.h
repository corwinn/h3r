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

//
//   It begins.
//
//   Basic definitions for the plug-in system. With such big a project, things
// need to get decoupled into sub-projects.
//
//   Who shall write the cloud save game manager plugin?
// Your code -> your rules.
//
//   You know AI?! - I'd love a challenging game!
//

//   C shall be supported I hope. Perhaps two headers would make things more
// readable.
//LATER R&D: Will there be an issue with C vs C++ plug-in protocol.

#ifndef _H3R_PLUGIN_H_
#define _H3R_PLUGIN_H_

#define public public:
#define private private:

// Avoiding runtime dependencies
typedef struct PlugInString
{
    const char * Text; // '\0'-terminated
    size_t Length;
    const char * Encoding; // '\0'-terminated ASCII
} PlugInString;

// Put here all the info the plugins could use. For example: version, game
// variant, pid or other unique instance identifier, map info, etc.
typedef struct PeerInfo
{
} PeerInfo;

//LATER add them all
enum PlugInType {PlugInTypeNone, PlugInTypeVFS, PlugInTypeAI}; // Reflection

// Put information here like: name, type, author, copyright, credits, whats
// supported, etc. e.g. this thing has to be extendable with new functionality
// w/o breaking exiting one
#ifdef __cplusplus
class PlugInInfo
{
    public virtual PlugInType GetType() { return PlugInTypeNone; }
    public virtual int RequiresSetup() { return 0; }
    public virtual void Setup() {}
};
#else
typedef struct
{
    enum PlugInType (*GetType)();
    int (*RequiresSetup)();
    void (*Setup)();
} PlugInInfo;
#endif

#ifdef __cplusplus
extern "C"
#endif
PlugInInfo * GetPlugInInfo(PeerInfo *);
typedef PlugInInfo * (*PGetPlugInInfo)(PeerInfo *);

// No need to include h3r_vfs.h. VFS shall adapt VFSPlugIn
#ifdef __cplusplus
class VFSPlugIn
{
};
#else
typedef struct
{
} VFSPlugIn;
#endif

#ifdef __cplusplus
extern "C"
#endif
VFSPlugIn * CreateVFSPlugIn(PeerInfo *);
typedef VFSPlugIn * (*PCreateVFSPlugIn)(PeerInfo *);

/* Use case (your mod has its resource in a "7z" archive):
     var obj = OS::LoadSharedObject ("MyMod_7zFS.dll");
     var info_proc = obj.GetProc<PGetPlugInInfo> ("GetPlugInInfo");
     var info = info_proc ();
     if (info-GetType () == PlugInType::VFS) {
         vfs_plugin = obj.GetProc<PCreateVFSPlugIn> ("CreateVFSPlugIn");
         VFS::RegisterPlugin (vfs_plugin ());
    }
*/

enum AIType {AITypeNone = 0, AITypeMap = 1, AITypeBattle = 2};
#ifdef __cplusplus
class AIPlugIn
{
    public int AvailiableTypes() { return AITypeMap | AITypeBattle; }

    // "Fighter", "Builder", "Explorer" are for replacement.
    // Conflicts (>1 "Builder" plug-ins for example) shall inquire the user.
    public PlugInString * Type() { return nullptr; }

    //LATER ...
};
#else
typedef struct
{
    int (*AvailiableTypes)();
    PlugInString * (*Type)();
} AIPlugIn;
#endif

#ifdef __cplusplus
extern "C"
#endif
AIPlugIn * CreateAIPlugIn(PeerInfo *);
typedef AIPlugIn * (*PCreateAIPlugIn)(PeerInfo *);

#undef public
#undef private

#endif
