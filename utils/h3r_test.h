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

#ifndef _H3R_TEST_H_
#define _H3R_TEST_H_

// test work - no frame

#define H3R_TEST
#include "h3r.h"
#include "h3r_test_unit.h"

#define H3R_TEST_UNIT(X) static Test_Unit test_unit_instance {#X};
#define H3R_TEST_RUN H3R_NS::test_unit_instance.Run ();

#define H3R_TEST_(X) static struct Test_##X final : Test_Proc { \
    Test_##X() { test_unit_instance.Add (this); } \
    void operator ()() override { \
        OS::Log_stdout ("%s: " #X EOL, test_unit_instance.Name ());

#define H3R_TEST_INSTANCE_NAME2(X) test_instance_##X
#define H3R_TEST_INSTANCE_NAME(X) H3R_TEST_INSTANCE_NAME2(X)
#define H3R_TEST_END } } H3R_TEST_INSTANCE_NAME(__LINE__);

#define H3R_TEST_ARE_EQUAL(A,B) H3R_ENSURE((A) == (B), "Equality test failed")
#define H3R_TEST_ARE_NOT_EQUAL(A,B) H3R_ENSURE((A) != (B), "!= test failed")
#define H3R_TEST_IS_NULL(P) H3R_ENSURE(nullptr == (P), "nullptr test failed")
#define H3R_TEST_IS_NOT_NULL(P) \
    H3R_ENSURE(nullptr != (P), "not nullptr test failed")
#define H3R_TEST_IS_TRUE(P) H3R_ENSURE(true == (P), "truth test failed")
#define H3R_TEST_IS_FALSE(P) H3R_ENSURE(false == (P), "false test failed")
#define H3R_TEST_EXCEPTION(E,C) { \
    bool e {false}; try { C (); } catch (E & exc) { e = true; } \
    H3R_ENSURE(true == e, "exception test failed") }

#endif