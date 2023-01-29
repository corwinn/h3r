//
//               "The Golden Crane flies for Tarmon Gai'Don."
//


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

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(File,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(Log,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_game.h"

#if _WIN32
#include "windows.h"
#include "h3r_gl.h"
#include "h3r_array.h"
static char ** a2b(PSTR lpCmdLine, int & argc)
{
    //TODO implement me
    static H3R_NS::Array<char> result {H3R_NS::OS::Strlen (lpCmdLine) + 4};
    H3R_NS::OS::Memmove ((char *) result, lpCmdLine, result.Length () - 4);
    argc = 1;
    static char * cptr {};
    return &cptr;
    return &(cptr = result);
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR lpCmdLine, INT)
{
    H3rGL_Init ();
    int argc {1};
    char ** argv {};
    //char ** argv = a2b (lpCmdLine, argc);
#else
int main(int argc, char ** argv)
{
#endif
    //TODO Path::GetDirName
    /*H3R_ENSURE(argc > 0, "Can't handle your OS - argc can't be < 1")
    H3R_NS::OS::Log_stdout ("Process: %s" EOL, argv[0]);
    // char * p = argv[0]; // the future base dir or work dir
    char * p  = "./";
    int len = H3R_NS::OS::Strlen (argv[0]);
    for (int i = len-1; i >= 0; i--)
        if ('/' == p[i] || '\\' == p[i] ) {
            p[i] = '\0';
            break;
        }
    H3R_NS::OS::Log_stdout ("WorkDir: %s" EOL, p);*/

    H3R_NS::Game game {"."};
    return game.Run (argc, argv);
}
