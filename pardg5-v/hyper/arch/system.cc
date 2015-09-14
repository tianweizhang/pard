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
#include "hyper/arch/PARDg5VLDomain.hh"
#include "hyper/arch/system.hh"
#include "mem/port_proxy.hh"
#include "params/X86PARDg5VSystem.hh"
#include "sim/byteswap.hh"
#include "sim/pseudo_inst.hh"

using namespace LittleEndianGuest;
using namespace X86ISA;

X86PARDg5VSystem::X86PARDg5VSystem(Params *p) :
    System(p), smbiosTable(p->smbios_table),
    mpFloatingPointer(p->intel_mp_pointer),
    mpConfigTable(p->intel_mp_table),
    rsdp(p->acpi_description_table_pointer),
    _numDomains(0),
    dumpstats_delay(p->dumpstats_delay), dumpstats_period(p->dumpstats_period),
    guest_startup_delay(p->guest_startup_delay)
{
    mgr = new PARDg5VGlobalManager(this);
    mgrl = new PARDg5VGlobalManagerListener(mgr, 8143);
    mgrl->listen();

    // Set back pointer to the system in all managed resources
    for (int x=0; x<params()->managed_resources.size(); x++)
        params()->managed_resources[x]->system(this);
}

void
X86PARDg5VSystem::init()
{
}

void
X86PARDg5VSystem::initState()
{
    System::initState();

    if (kernel)
        fatal("PARDg5-V host not support kernel, assign kernel to guest.\n");

    // Stop all thread contexts
    for (int i=0; i<threadContexts.size(); i++)
        threadContexts[i]->suspend();

    // init BSP state of each X86PARDg5VGuest
    for (int i=0; i<domains.size(); i++) {
        domains[i]->initBSPState(threadContexts[i]);
        threadContexts[i]->activate();
    }

    if (dumpstats_delay != -1)
        PseudoInst::dumpresetstats(threadContexts[0], dumpstats_delay, dumpstats_period);
}

void
X86PARDg5VSystem::loadState(Checkpoint *cp)
{
    System::loadState(cp);
    if (dumpstats_delay != -1) {
        PseudoInst::dumpresetstats(threadContexts[0],
            dumpstats_delay, dumpstats_period);
    }
}

int
X86PARDg5VSystem::registerPARDg5VLDomain(PARDg5VLDomain *domain)
{
    int id;

    for (id = 0; id < domains.size(); id++) {
        if (!domains[id])
            break;
    }

    if (domains.size() <= id)
        domains.resize(id + 1);

    domains[id] = domain;
    _numDomains++;

    DPRINTFN("register PARDg5VLDomain: %d\n", id);

    return id;
}

PARDg5VLDomain *
X86PARDg5VSystem::createPARDg5VLDom()
{
    return new PARDg5VLDomain(this, 1, 0x80000000);
}

X86PARDg5VSystem::~X86PARDg5VSystem()
{
    delete smbiosTable;
    delete mgr;
    delete mgrl;
}

X86PARDg5VSystem *
X86PARDg5VSystemParams::create()
{
    return new X86PARDg5VSystem(this);
}

