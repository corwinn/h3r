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

//c clang++ -std=c++11 -Wall -Wextra -Wshadow -O0 -g -gdwarf-4 -fsanitize=address,undefined,integer,leak -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -UH3R_MM -fno-exceptions -fno-threadsafe-statics main.a parse_map_ffd.cpp -o parse_map_ffd -lz

// run:
//  #01 - ~40 min.; tons of errors; parsed: 3137/6167 maps; good
//  #02 - ~50 min.; ...           ; parsed: 4758/6167 ... ; ...

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(File,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_ffdnode.h"

int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: parse_map_ffd lodfile\n");

    Dbg.Enabled = false;
    auto test = H3R_NS::FFD::File2Tree ("map_grammar", argv[1]);
    Dbg.Enabled = true;
    Dbg << "nodes: " << test->NodeCount ()
        << ", bytes: " << test->NodeCount () * sizeof(*test) << EOL;
    H3R_DESTROY_OBJECT(test, FFDNode)

    return 0;
}