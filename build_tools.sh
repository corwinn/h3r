# /**** BEGIN LICENSE BLOCK ****
#
# BSD 3-Clause License
#
# Copyright (c) 2021-2023, the wind.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# **** END LICENCE BLOCK ****/

#!/bin/bash
set -x

CXX=${CXX:-clang++}
L="-DLIST_ONLY"
F="-fsanitize=address,undefined,integer -fvisibility=hidden \
 -fno-exceptions -fno-threadsafe-statics"
I="-std=c++11 -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG"
OBJ="main.a h3r_game.o"

$CXX $I $F         parse_map.cpp -o parse_map -lz
$CXX $I $F         unpack_lod.cpp -o unpack_lod -lz
$CXX $I $F $L      unpack_lod.cpp -o list_lod -lz
$CXX $I $F    $OBJ unpack_snd.cpp -o unpack_snd
$CXX $I $F $L $OBJ unpack_snd.cpp -o list_snd
$CXX $I $F    $OBJ unpack_vid.cpp -o unpack_vid
$CXX $I $F $L $OBJ unpack_vid.cpp -o list_vid
$CXX $I $F    $OBJ parse_pcx.cpp -o parse_pcx
$CXX $I $F    $OBJ parse_def.cpp -o parse_def