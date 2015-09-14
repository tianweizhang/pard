/*
 * Copyright (c) 2007 The Hewlett-Packard Development Company
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Gabe Black
 */

#ifndef __ARCH_X86_REMOTEGDB_HH__
#define __ARCH_X86_REMOTEGDB_HH__

#include "arch/x86/types.hh"
#include "base/remote_gdb.hh"

class System;
class ThreadContext;

namespace X86ISA
{
    class RemoteGDB : public BaseRemoteGDB
    {
      protected:
        enum RegisterContants
        {
            /* Below are 8-byte length */
            GDB_AX,                 /* 0 */
            GDB_BX,                 /* 1 */
            GDB_CX,                 /* 2 */
            GDB_DX,                 /* 3 */
            GDB_SI,                 /* 4 */
            GDB_DI,                 /* 5 */
            GDB_BP,                 /* 6 */
            GDB_SP,                 /* 7 */
            GDB_R8,                 /* 8 */
            GDB_R9,                 /* 9 */
            GDB_R10,                /* 10 */
            GDB_R11,                /* 11 */
            GDB_R12,                /* 12 */
            GDB_R13,                /* 13 */
            GDB_R14,                /* 14 */
            GDB_R15,                /* 15 */
            GDB_PC,                 /* 16 */
            /* Below are 4byte length */
            GDB_CS_PS,
            GDB_DS_SS,
            GDB_FS_ES,
            GDB_GS,
            //GDB_PS,                 /* 17 */
            //GDB_CS,                 /* 18 */
            //GDB_SS,                 /* 19 */
            //GDB_DS,                 /* 20 */
            //GDB_ES,                 /* 21 */
            //GDB_FS,                 /* 22 */
            //GDB_GS,                 /* 23 */
            NumGDBRegs
            //XXX fill this in
        };

      public:
        RemoteGDB(System *system, ThreadContext *context);

        bool acc(Addr addr, size_t len);

      protected:
        void getregs();
        void setregs();

        void clearSingleStep();
        void setSingleStep();

        Addr nextBkpt;
        Addr notTakenBkpt;
        Addr takenBkpt;
    };
}

#endif // __ARCH_X86_REMOTEGDB_HH__
