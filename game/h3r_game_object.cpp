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

#include "h3r.h"
#include "h3r_game_object.h"
#include "h3r_list.h"

#include <new>

H3R_NAMESPACE

// The MM could be off yet, so its off-limits <=> random static init.
//TODO resolve this "static init. is random" issue; there shall be functions
//     called at main if needs be;

namespace { struct ObjectFactory
{
    private: List<h3rMapVersion> _map_version_hash {};
    private: List<List<Object *> *> _obj_factories {};
    private: ObjectFactory() {}
    public: ~ObjectFactory()
    {
        for (int i = 0; i < _obj_factories.Count (); i++) {
            if (nullptr == _obj_factories[i]) continue;
            for (int j = 0; j < _obj_factories[i]->Count (); j++)
                if (nullptr != (*_obj_factories[i])[j])
                    (*_obj_factories[i])[j]->~Object (),
                    OS::Mfree ((*_obj_factories[i])[j]);
            _obj_factories[i]->~List<Object *>();
            OS::Mfree (_obj_factories[i]);
        }
    }
    private: int const _distinct_types {255};
    private: int const _distinct_versions {6};
    private: int MapVersionHash(const h3rMapVersion v)
    {
        // They are 3; double that; you don't need a map still.
        for (int i = 0; i < _map_version_hash.Count (); i++)
            if (v == _map_version_hash[i]) return i;
        return -1;
    }
    private: Object * GetObj(h3rMapVersion v, h3rObjRef r)
    {
        int map_version = One ().MapVersionHash (v);
        if (-1 == map_version)
            // either unsupported map version or a corrupted map
            return nullptr;
        if (! _obj_factories[map_version])
            // either unsupported obj type or a corrupted map
            return nullptr;
        return (*_obj_factories[map_version])[r];
    }
    private: void BindObj(Object * obj, h3rMapVersion v, h3rObjType t)
    {
        int map_version = MapVersionHash (v);
        if (-1 == map_version) {
            H3R_ENSURE(_map_version_hash.Count ()
                < _distinct_versions,
                "bug: ObjectFactory::Bind() something fishy is going on")
            _map_version_hash.Add (v);
            map_version = _map_version_hash.Count () - 1;
        }
        if (map_version >= static_cast<int>(_obj_factories.Count ())) {
            List<Object *> * list;
            OS::Malloc (list);
            new (list) List<Object *> {_distinct_types};
            _obj_factories.Add (list);
        }
        H3R_ENSURE(map_version >= 0
            && map_version < static_cast<int>(_obj_factories.Count ()),
            "bug: ObjectFactory::Bind() something fishy is going on")
        (*_obj_factories[map_version])[t] = obj;
    }
    private: static inline ObjectFactory & One()
    {
        static ObjectFactory m;
        return m;
    }
    public: static inline Object * Get(h3rMapVersion v, h3rObjRef r)
    {
        return One ().GetObj (v, r);
    }
    public: static inline void Bind(Object * obj, h3rMapVersion v, h3rObjType t)
    {
        One ().BindObj (obj, v, t);
    }
};}// namespace { struct ObjectFactory

template <typename T> /*static*/ void Object::Register(
    h3rMapVersion mv, h3rObjType t)
{
    T * i;
    OS::Malloc (i);
    new (i) T {};
    ObjectFactory::Bind (i, mv, t);
}

Object * Object::Create(Stream & s, h3rMapVersion mv, const Object & type)
{
    Object * object_factory = ObjectFactory::Get (mv, type.Sprite ());
    return object_factory->ContinueCreating (s);
}

NAMESPACE_H3R
