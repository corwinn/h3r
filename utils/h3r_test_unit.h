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

#ifndef _H3R_TEST_UNIT_H_
#define _H3R_TEST_UNIT_H_

// test work - no frame

#include "h3r.h"
#include "h3r_test_proc.h"
#include "h3r_os.h"

H3R_NAMESPACE

class Test_Unit
{
    private static const int _MAX_PROCS {100};
    private Test_Proc * _procs[_MAX_PROCS];
    private const char * _name;
    private int _ok {}, _all {};
    public const char * Name () { return _name; }
    public Test_Unit(const char * name) : _name {name} {}
    public ~Test_Unit()
    {
        OS::Log_stdout ("================" EOL "Tests: %4d/%4d" EOL,
            _ok, _all);
        H3R_ENSURE(_ok == _all, "Some test(s) failed")
    }
    public void Add(Test_Proc * proc)
    {
        H3R_ENSURE(_all >= 0, "Index out of range.")
        H3R_ENSURE(_all < _MAX_PROCS-1, "Please, create another test unit.")
        _procs[_all++] = proc;
    }
    public void Run()
    {
        for (int p = 0; p < _all; p++) {
            H3R_ENSURE(nullptr != _procs[p], "null test proc pointer")
            try { (*_procs[p]) (); }
            catch (Exception & exc) {
                OS::Log_stderr ("Unhandled exception: %s:%d: \"%s\"" EOL,
                    exc.File (), exc.Line (), exc.Msg ());
                OS::Exit (OS::EXIT_WITH_ERROR);
            }
            catch (...) {
                OS::Log_stderr ("Unhandled unkown exception on test %d" EOL,
                    p);
                OS::Exit (OS::EXIT_WITH_ERROR);
            }
            _ok++;
        }
    }
}; // class Test_Unit

NAMESPACE_H3R

#endif