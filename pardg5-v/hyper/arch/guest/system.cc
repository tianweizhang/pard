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

#include "arch/x86/bios/intelmp.hh"
#include "arch/x86/bios/smbios.hh"
#include "arch/x86/regs/misc.hh"
#include "arch/x86/isa_traits.hh"
#include "arch/vtophys.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "base/intmath.hh"
#include "base/trace.hh"
#include "cpu/thread_context.hh"
#include "debug/Loader.hh"
#include "hyper/arch/system.hh"
#include "hyper/arch/guest/system.hh"
#include "hyper/arch/PARDg5VLDomain.hh"
#include "mem/port_proxy.hh"
#include "params/X86PARDg5VGuest.hh"
#include "sim/byteswap.hh"

using namespace LittleEndianGuest;
using namespace X86ISA;

X86PARDg5VGuest::X86PARDg5VGuest(Params *p) :
    SimObject(p), system(p->system), _guestID(-1),
    physProxy(system->getSystemPort(), system->cacheLineSize(), _guestID),
    loadAddrMask(p->load_addr_mask),
    loadAddrOffset(p->load_offset),
    smbiosTable(p->smbios_table),
    mpFloatingPointer(p->intel_mp_pointer),
    mpConfigTable(p->intel_mp_table),
    rsdp(p->acpi_description_table_pointer)
{
    if (FullSystem) {
        kernelSymtab = new SymbolTable;
        if (!debugSymbolTable)
            debugSymbolTable = new SymbolTable;
    }

    if (FullSystem) {
        if (params()->kernel == "") {
            inform("No kernel set for full system simulation. "
                   "Assuming you know what you're doing\n");

            kernel = NULL;
        } else {
            // Get the kernel code
            kernel = createObjectFile(params()->kernel);
            inform("kernel located at: %s", params()->kernel);

            if (kernel == NULL)
                fatal("Could not load kernel file %s", params()->kernel);

            // setup entry points
            kernelStart = kernel->textBase();
            kernelEnd = kernel->bssBase() + kernel->bssSize();
            kernelEntry = kernel->entryPoint();

            // load symbols
            if (!kernel->loadGlobalSymbols(kernelSymtab))
                fatal("could not load kernel symbols\n");

            if (!kernel->loadLocalSymbols(kernelSymtab))
                fatal("could not load kernel local symbols\n");

            if (!kernel->loadGlobalSymbols(debugSymbolTable))
                fatal("could not load kernel symbols\n");

            if (!kernel->loadLocalSymbols(debugSymbolTable))
                fatal("could not load kernel local symbols\n");

            // Loading only needs to happen once and after memory system is
            // connected so it will happen in initState()
        }
    }
}

void
X86PARDg5VGuest::init()
{
   new PARDg5VLDomain(this);
}

X86PARDg5VGuest::~X86PARDg5VGuest()
{
    delete smbiosTable;
}

X86PARDg5VGuest *
X86PARDg5VGuestParams::create()
{
    return new X86PARDg5VGuest(this);
}

