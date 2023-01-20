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

MAKEFLAGS += rR

APP = main
PLATFORM ?= posix
CC ?= clang
CXX ?= clang++
H3R_TEST ?=
_W  = -Wall -Wextra -Wshadow
_O  = -O0 -g -DH3R_DEBUG -DH3R_MM -fno-exceptions -fno-threadsafe-statics \
 $(H3R_TEST)
_F  = -fsanitize=address,undefined,integer,leak -fvisibility=hidden
#TODO release build _F = -fvisibility=hidden -fno-rtti
_L = -Wl,--as-needed -lpthread -lz -lSDL2 -lSDL2_mixer
_I  = -I. -Ios -Ios/$(PLATFORM) -Iutils -Iui -Istream -Iasync -Igame \
 `pkg-config --cflags sdl2`

CXXFLAGS = $(_I) -std=c++11 $(_O) $(_F) $(_W)
SRC = $(wildcard ./*/*.cpp)
SRC += $(wildcard os/$(PLATFORM)/*.cpp)
SRC := $(filter-out ./prior_publish/%,$(SRC))
OBJ = $(patsubst %.cpp,%.o,$(SRC))

$(APP): main.a main.o h3r_game.o
	$(CXX) $(_F) $^ -o $@ $(_L)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

main.a: $(OBJ)
	ar rcs $@ $(OBJ)

clean:
	find -type f -iname "*~" -delete -o -iname "*.o" -delete
	rm -f main.a main.o main

TEST:
	@echo $(SRC)
	@echo $(OBJ)

cake: universe

starship: together
