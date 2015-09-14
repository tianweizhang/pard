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

#include <sys/signal.h>
#include <unistd.h>

#include <string>

#include "arch/x86/insts/badmicroop.hh"
#include "arch/x86/decoder.hh"
#include "arch/x86/pagetable.hh"
#include "arch/x86/pagetable_walker.hh"
#include "arch/x86/predecoder.hh"
#include "arch/x86/registers.hh"
#include "arch/x86/remote_gdb.hh"
#include "arch/x86/utility.hh"
#include "arch/x86/vtophys.hh"
#include "base/remote_gdb.hh"
#include "base/socket.hh"
#include "base/trace.hh"
#include "cpu/static_inst.hh"
#include "cpu/thread_context.hh"
#include "debug/GDBAcc.hh"
#include "debug/GDBMisc.hh"
#include "sim/full_system.hh"

using namespace std;
using namespace X86ISA;

static inline Addr
truncPage(Addr addr)
{
    return addr & ~(PageBytes - 1);
}

static inline Addr
roundPage(Addr addr)
{
    return (addr + PageBytes - 1) & ~(PageBytes - 1);
}

static inline bool
virtvalid(ThreadContext *tc, Addr vaddr)
{
    Walker *walker = tc->getDTBPtr()->getWalker();
    unsigned logBytes;
    Fault fault = walker->startFunctional(
            tc, vaddr, logBytes, BaseTLB::Read);
    if (fault != NoFault)
        return false;
    return true;
}

RemoteGDB::RemoteGDB(System *_system, ThreadContext *c)
    : BaseRemoteGDB(_system, c, NumGDBRegs)
{
}

/*
 * Determine if the mapping at va..(va+len) is valid.
 */
bool RemoteGDB::acc(Addr va, size_t len)
{
    if (!FullSystem) {
        panic("X86 RemoteGDB only support full system.\n");
    }

    Addr last_va;
    va       = truncPage(va);
    last_va  = roundPage(va + len);

    do  {
        if (!virtvalid(context, va)) {
             return false;
        }
        va += PageBytes;
    } while (va < last_va);

    DPRINTF(GDBAcc, "acc:   %#x mapping is valid\n", va);
    return true;
}

/*
 * Translate the kernel debugger register format into the GDB register
 * format.
 */
void RemoteGDB::getregs()
{
    DPRINTF(GDBAcc, "getregs in X86 RemoteGDB.\n");

    memset(gdbregs.regs, 0, gdbregs.bytes());
    gdbregs.regs[GDB_AX] = context->readIntReg(X86ISA::INTREG_RAX);
    gdbregs.regs[GDB_BX] = context->readIntReg(X86ISA::INTREG_RBX);
    gdbregs.regs[GDB_CX] = context->readIntReg(X86ISA::INTREG_RCX);
    gdbregs.regs[GDB_DX] = context->readIntReg(X86ISA::INTREG_RDX);
    gdbregs.regs[GDB_SI] = context->readIntReg(X86ISA::INTREG_RSI);
    gdbregs.regs[GDB_DI] = context->readIntReg(X86ISA::INTREG_RDI);
    gdbregs.regs[GDB_BP] = context->readIntReg(X86ISA::INTREG_RBP);
    gdbregs.regs[GDB_SP] = context->readIntReg(X86ISA::INTREG_RSP);
    gdbregs.regs[GDB_R8] = context->readIntReg(X86ISA::INTREG_R8);
    gdbregs.regs[GDB_R9] = context->readIntReg(X86ISA::INTREG_R9);
    gdbregs.regs[GDB_R10] = context->readIntReg(X86ISA::INTREG_R10);
    gdbregs.regs[GDB_R11] = context->readIntReg(X86ISA::INTREG_R11);
    gdbregs.regs[GDB_R12] = context->readIntReg(X86ISA::INTREG_R12);
    gdbregs.regs[GDB_R13] = context->readIntReg(X86ISA::INTREG_R13);
    gdbregs.regs[GDB_R14] = context->readIntReg(X86ISA::INTREG_R14);
    gdbregs.regs[GDB_R15] = context->readIntReg(X86ISA::INTREG_R15);

    gdbregs.regs[GDB_PC] = context->pcState().pc();

    gdbregs.regs[GDB_CS_PS] = context->readMiscReg(X86ISA::MISCREG_CS) << 32 |
                                       context->readMiscReg(X86ISA::MISCREG_RFLAGS);
    gdbregs.regs[GDB_DS_SS] = context->readMiscReg(X86ISA::MISCREG_DS) << 32 |
                                       context->readMiscReg(X86ISA::MISCREG_SS);
    gdbregs.regs[GDB_FS_ES] = context->readMiscReg(X86ISA::MISCREG_FS) << 32 |
                                       context->readMiscReg(X86ISA::MISCREG_ES);
    gdbregs.regs[GDB_GS]    = context->readMiscReg(X86ISA::MISCREG_GS);
}

void RemoteGDB::setregs()
{
    DPRINTF(GDBAcc, "setregs in X86 RemoteGDB.\n");
    warn("X86ISA::RemoteGDB::setregs() not fully tested, this may damage your simulated system.");
    context->setIntReg(X86ISA::INTREG_RAX, gdbregs.regs[GDB_AX]);
    context->setIntReg(X86ISA::INTREG_RBX, gdbregs.regs[GDB_BX]);
    context->setIntReg(X86ISA::INTREG_RCX, gdbregs.regs[GDB_CX]);
    context->setIntReg(X86ISA::INTREG_RDX, gdbregs.regs[GDB_DX]);
    context->setIntReg(X86ISA::INTREG_RSI, gdbregs.regs[GDB_SI]);
    context->setIntReg(X86ISA::INTREG_RDI, gdbregs.regs[GDB_DI]);
    context->setIntReg(X86ISA::INTREG_RBP, gdbregs.regs[GDB_BP]);
    context->setIntReg(X86ISA::INTREG_RSP, gdbregs.regs[GDB_SP]);
    context->setIntReg(X86ISA::INTREG_R8,  gdbregs.regs[GDB_R8]);
    context->setIntReg(X86ISA::INTREG_R9,  gdbregs.regs[GDB_R9]);
    context->setIntReg(X86ISA::INTREG_R10, gdbregs.regs[GDB_R10]);
    context->setIntReg(X86ISA::INTREG_R11, gdbregs.regs[GDB_R11]);
    context->setIntReg(X86ISA::INTREG_R12, gdbregs.regs[GDB_R12]);
    context->setIntReg(X86ISA::INTREG_R13, gdbregs.regs[GDB_R13]);
    context->setIntReg(X86ISA::INTREG_R14, gdbregs.regs[GDB_R14]);
    context->setIntReg(X86ISA::INTREG_R15, gdbregs.regs[GDB_R15]);
    context->pcState().set(gdbregs.regs[GDB_PC]);
    context->setMiscReg(X86ISA::MISCREG_RFLAGS, bits(gdbregs.regs[GDB_CS_PS], 31, 0));
    context->setMiscReg(X86ISA::MISCREG_CS,     bits(gdbregs.regs[GDB_CS_PS], 63, 32));
    context->setMiscReg(X86ISA::MISCREG_SS,     bits(gdbregs.regs[GDB_DS_SS], 31,  0));
    context->setMiscReg(X86ISA::MISCREG_DS,     bits(gdbregs.regs[GDB_DS_SS], 63, 32));
    context->setMiscReg(X86ISA::MISCREG_ES,     bits(gdbregs.regs[GDB_FS_ES], 31,  0));
    context->setMiscReg(X86ISA::MISCREG_FS,     bits(gdbregs.regs[GDB_FS_ES], 63, 32));
    context->setMiscReg(X86ISA::MISCREG_GS,     bits(gdbregs.regs[GDB_GS],    31,  0));
}


void RemoteGDB::clearSingleStep()
{
    DPRINTF(GDBMisc, "clearSingleStep bt_addr=%#x nt_addr=%#x\n",
            takenBkpt, notTakenBkpt);

    if (takenBkpt != 0)
        clearTempBreakpoint(takenBkpt);

    if (notTakenBkpt != 0)
        clearTempBreakpoint(notTakenBkpt);
}

void RemoteGDB::setSingleStep()
{
    PCState pc = context->pcState();
    PCState bpc;
    bool set_bt = false;

    // User was stopped at pc, e.g. the instruction at pc was not
    // executed.
    Decoder *decoder = context->getDecoderPtr();
    X86Predecoder predecoder(read<MachInst>(pc.pc()), decoder);
    ExtMachInst &emi = predecoder.getExtMachInst();

    // we assert X86Inst are all MacroopInst
    StaticInstPtr si = decoder->decodeInst(emi);
    assert (si->isMacroop());

    pc.size(predecoder.getInstSize());
    pc.npc(pc.pc() + pc.size());

    MicroPC microPC = 0;
    StaticInstPtr msi = NULL;
    while ((msi = si->fetchMicroop(microPC++)) != badMicroop)
    {
        if (msi->getName() == "wrip") {
/*
            uint64_t SrcReg1 = context->readIntReg(msi->srcRegIdx(0));
            uint64_t SrcReg2 = context->readIntReg(msi->srcRegIdx(1));
            uint64_t CSBase  = context->readIntReg(msi->srcRegIdx(2));
            DPRINTF(GDBMisc, "found a branch inst @ %#x\n", pc.pc());
            DPRINTF(GDBMisc, "  SrcReg1 = %#x;  srcReg2 = %#x;  CSBase= %#x\n",
                    SrcReg1, SrcReg2, CSBase);
*/
        }
        else if (msi->getName() == "wripi") {
            DPRINTF(GDBMisc, "found a branch inst @ %#x\n", pc.pc());
        }
/*
        if (bpc.pc() != pc.npc())
            set_bt = true;
*/
    }


    setTempBreakpoint(notTakenBkpt = pc.npc());

    if (set_bt)
        setTempBreakpoint(takenBkpt = bpc.pc());

    DPRINTF(GDBMisc, "setSingleStep bt_addr=%#x nt_addr=%#x\n",
            takenBkpt, notTakenBkpt);
}

