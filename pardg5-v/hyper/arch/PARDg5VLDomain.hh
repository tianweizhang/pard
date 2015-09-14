/*
 * Copyright (c) 2014 ACS, ICT
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

#ifndef __HYPER_ARCH_PARDG5VLDOMAIN_HH__
#define __HYPER_ARCH_PARDG5VLDOMAIN_HH__

#include "hyper/base/pardg5v.hh"
#include "hyper/mem/port_proxy.hh"

namespace X86ISA
{
    class E820Table;
    namespace SMBios
    {
        class SMBiosTable;
    }
    namespace IntelMP
    {
        class FloatingPointer;
        class ConfigTable;
    }
    namespace ACPI
    {
        class RSDT;
        class XSDT;
        class RSDP;
    }
}

class X86PARDg5VGuest;
class X86PARDg5VSystem;

// Logical Domain
class PARDg5VLDomain
{
  protected:

    X86PARDg5VGuest *guest;

    X86PARDg5VSystem *system;

    LDomID domID;

  public:

    PARDg5VLDomain(X86PARDg5VGuest *guest);
    PARDg5VLDomain(X86PARDg5VSystem *system, int num_cpus, Addr memsize);
    virtual ~PARDg5VLDomain();

    void initBSPState(ThreadContext *tcBSP);
    void prepareLinuxEnv(ThreadContext *tcBSP);

    LDomID getLDomID() const { return domID; }

  protected:

    /** Port to physical memory used for writing object files into ram at
     * boot.*/
    PARDg5VPortProxy physProxy;

  protected:

    /**
     * Kernel
     */

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
    /** Mask that should be anded for binary/symbol loading. */
    Addr loadAddrMask;
    /** Offset that should be used for binary/symbol loading. */
    Addr loadAddrOffset;

    // Linux Env
    std::string commandLine;
    X86ISA::E820Table * e820Table;

    // BIOS
    X86ISA::SMBios::SMBiosTable * smbiosTable;
    X86ISA::IntelMP::FloatingPointer * mpFloatingPointer;
    X86ISA::IntelMP::ConfigTable * mpConfigTable;
    X86ISA::ACPI::RSDP * rsdp;

    void writeOutSMBiosTable(Addr header,
            Addr &headerSize, Addr &tableSize, Addr table = 0);

    void writeOutMPTable(Addr fp,
            Addr &fpSize, Addr &tableSize, Addr table = 0);

};

#endif	// __HYPER_ARCH_PARDG5VLDOMAIN_HH__

