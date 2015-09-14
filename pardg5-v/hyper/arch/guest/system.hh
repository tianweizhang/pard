/*
 * Copyright (c) 2007 ACS, ICT
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
 * Authors: Jiuyue Ma
 */

#ifndef __HYPER_ARCH_GUEST_SYSTEM_HH__
#define __HYPER_ARCH_GUEST_SYSTEM_HH__

#include <string>
#include <vector>

#include "base/loader/symtab.hh"
#include "cpu/pc_event.hh"
#include "hyper/mem/port_proxy.hh"
#include "kern/system_events.hh"
#include "params/X86PARDg5VGuest.hh"
#include "sim/full_system.hh"
#include "sim/sim_object.hh"


class ObjectFile;

namespace X86ISA
{
    namespace SMBios
    {
        class SMBiosTable;
    }
    namespace IntelMP
    {
        class FloatingPointer;
        class ConfigTable;
    }
}

class X86PARDg5VGuest : public SimObject
{
  private:
    friend class PARDg5VLDomain;

  public:
    typedef X86PARDg5VGuestParams Params;
    X86PARDg5VGuest(Params *p);
    virtual ~X86PARDg5VGuest();

    void init();

  public:

    /** System we are currently operating in. */
    X86PARDg5VSystem *system;

    /** ID of this guest */
    int _guestID;

    /** Port to physical memory used for writing object files into ram at
     * boot.*/
    PARDg5VPortProxy physProxy;

    /** kernel symbol table */
    SymbolTable *kernelSymtab;

    /** Object pointer for the kernel code */
    ObjectFile *kernel;

    /** Begining of kernel code */
    Addr kernelStart;

    /** End of kernel code */
    Addr kernelEnd;

    /** Entry point in the kernel to start at */
    Addr kernelEntry;

    /** Mask that should be anded for binary/symbol loading.
     * This allows one two different OS requirements for the same ISA to be
     * handled.  Some OSes are compiled for a virtual address and need to be
     * loaded into physical memory that starts at address 0, while other
     * bare metal tools generate images that start at address 0.
     */
    Addr loadAddrMask;

    /** Offset that should be used for binary/symbol loading.
     * This further allows more flexibily than the loadAddrMask allows alone in
     * loading kernels and similar. The loadAddrOffset is applied after the
     * loadAddrMask.
     */
    Addr loadAddrOffset;

  protected:

    X86ISA::SMBios::SMBiosTable * smbiosTable;
    X86ISA::IntelMP::FloatingPointer * mpFloatingPointer;
    X86ISA::IntelMP::ConfigTable * mpConfigTable;
    X86ISA::ACPI::RSDP * rsdp;

    const Params *params() const { return (const Params *)_params; }

    virtual Addr fixFuncEventAddr(Addr addr)
    {
        //XXX This may eventually have to do something useful.
        return addr;
    }
};

#endif

