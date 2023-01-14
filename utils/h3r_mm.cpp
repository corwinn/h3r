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

#include "h3r_os.h"
#include "h3r_mm.h"

H3R_NAMESPACE
namespace OS {

// MM shall be the only singleton intialized this way!
/*static*/ MM & MM::One () { static MM m; return m; }

MM::MM()
{
    static bool the_way_is_shut {true};
    H3R_ENSURE(the_way_is_shut, "I'm a c++ singleton!")
    the_way_is_shut = false;
    // H3R_ENSURE(! MM::_one, "I'm a c++ singleton!")
    // MM::_one = this;
    OS::Log_stdout ("MM::MM ()" EOL);
}

MM::~MM()
{
    Log_stdout ("Allocated %d entries, total of %d bytes (Entry[] "
                ": %d bytes, user: %d bytes)" EOL,
        _c, _c * sizeof(Entry) + _user_bytes, _c * sizeof(Entry),
        _user_bytes);
    Log_stdout ("Allocations: %d, frees: %d" EOL, _a, _f);
    Log_stdout ("Current bytes: %d" EOL, _current_bytes);
    for (int i = 0; i < _n; i++)
        OS::Mfree (_e[i].p);
    _n = _c = _a = _f = _current_bytes = _user_bytes = 0;
    Mfree (_e);
}

} // namespace OS
NAMESPACE_H3R
