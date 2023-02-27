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

//c clang++ -std=c++11 -Wall -Wextra -Wshadow -O0 -g -gdwarf-4 -fsanitize=address,undefined,integer,leak -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -Iffd -O0 -g -DH3R_DEBUG -UH3R_MM -fno-exceptions -fno-threadsafe-statics main.a parse_map_ffd.cpp -o parse_map_ffd -lz

// run:
//  #01 - ~40 min.; tons of errors; parsed: 3137/6167 maps; good
//  #02 - ~50 min.; ...           ; parsed: 4758/6167 ... ; ...
//  #03 - ~44 min.; ...           ; parsed: shame on me/6167 ... ; regression
//  #04 - ~57 min.; ...           ; parsed: 6167/6167 ... ; done

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(File,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_ffdnode.h"

int main(int argc, char ** argv)
{
    if (3 != argc)
        return printf ("usage: parse_map_ffd descritpion_file h3m_file\n");

    H3R_NS::FFD ffd {};

    Dbg.Enabled = false;
    auto test = ffd.File2Tree (argv[1], argv[2]);
    Dbg.Enabled = true;
    Dbg << "nodes: " << test->NodeCount ()
        << ", bytes: " << test->NodeCount () * sizeof(*test) << EOL;
    // test->PrintTree ();
    H3R_DESTROY_OBJECT(test, FFDNode)

    return 0;
}