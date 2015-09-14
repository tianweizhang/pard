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

#ifndef __HYPER_ARCH_X86_SYSTEM_HH__
#define __HYPER_ARCH_X86_SYSTEM_HH__

#include <string>
#include <vector>

#include "base/loader/symtab.hh"
#include "cpu/pc_event.hh"
#include "hyper/arch/glomgr.hh"
#include "kern/system_events.hh"
#include "params/X86PARDg5VSystem.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"

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

class PARDg5VLDomain;

class X86PARDg5VSystem : public System
{
  public:
    typedef X86PARDg5VSystemParams Params;
    X86PARDg5VSystem(Params *p);
    ~X86PARDg5VSystem();

  public:

    virtual void init();
    virtual void initState();
    virtual void loadState(Checkpoint *cp);

  public:

    int registerPARDg5VLDomain(PARDg5VLDomain *domain);

    PARDg5VLDomain * createPARDg5VLDom();

    int numPARDg5VLDomain() { return _numDomains; }
    PARDg5VLDomain * getPARDg5VLDomain(LDomID id) {
        return (id < _numDomains) ? domains[id] : NULL;
    }

  protected:

    X86ISA::SMBios::SMBiosTable * smbiosTable;
    X86ISA::IntelMP::FloatingPointer * mpFloatingPointer;
    X86ISA::IntelMP::ConfigTable * mpConfigTable;
    X86ISA::ACPI::RSDP * rsdp;

    std::vector<PARDg5VLDomain *> domains;
    int _numDomains;

    Tick dumpstats_delay;
    Tick dumpstats_period;
    Tick guest_startup_delay;

    const Params *params() const { return (const Params *)_params; }

    virtual Addr fixFuncEventAddr(Addr addr)
    {
        //XXX This may eventually have to do something useful.
        return addr;
    }

  public:

    PARDg5VGlobalManager *mgr;

  protected:
    PARDg5VGlobalManagerListener *mgrl;

};

#endif

