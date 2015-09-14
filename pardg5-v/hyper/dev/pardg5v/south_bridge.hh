/*
 * Copyright (c) 2008 The Regents of The University of Michigan
 * All rights reserved.
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

#ifndef __HYPER_DEV_PARDG5V_SOUTH_BRIDGE_HH__
#define __HYPER_DEV_PARDG5V_SOUTH_BRIDGE_HH__

#include "params/PARDg5VSouthBridge.hh"
#include "sim/sim_object.hh"

namespace X86ISA
{
    class I8254P5V;
    class Cmos;
    class I82094AAP5V;
}

class PARDg5VSouthBridge : public SimObject
{
  protected:
    Platform * platform;

  public:
    X86ISA::I8254P5V * pit;
    X86ISA::Cmos * cmos;
    X86ISA::I82094AAP5V * ioApic;

  public:
    typedef PARDg5VSouthBridgeParams Params;
    PARDg5VSouthBridge(const Params *p);

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }
};

#endif //__DEV_X86_SOUTH_BRIDGE_HH__
