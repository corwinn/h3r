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

PLATFORM=${PLATFORM:-posix}
STD="-std=c++11"
W="-Wall -Wextra -Wshadow -Wvariadic-macros"
O="-O0 -g -fno-omit-frame-pointer -fexceptions"
RTC="-fsanitize=address,undefined,integer"
C="-fprofile-instr-generate -fcoverage-mapping"
I="-DH3R_DEBUG -UH3R_MM -I. -Ios -Iutils -Iui -Ios/${PLATFORM}"
F="${1:?}"
FO="test_${F%%.*}"
FO="${FO//\//_}"
# the test requires more and more deps; this should be fixed ASAP;
if [ "$F" != "$FO" ]; then
  make clean && make H3R_TEST="-DH3R_TEST -fexceptions -UH3R_MM "
  clang++ $STD $W $O $RTC $C $I main.a h3r_game.o -x c++ "$F" -o "$FO" \
  -Wl,--as-needed -lz -lSDL2 -lSDL2_mixer -lGL
else
  echo no no no - the compiler will replace your input w/o a warning
  exit 1
fi
