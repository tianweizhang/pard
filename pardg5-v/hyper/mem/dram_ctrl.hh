/*
 * Copyright (c) 2014 ACS, ICT
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

/**
 * @file
 * PARDg5VDRAMCtrl declaration
 */

#ifndef __HYPER_MEM_PARDG5V_DRAM_CTRL_HH__
#define __HYPER_MEM_PARDG5V_DRAM_CTRL_HH__

#include <deque>
#include <vector>

#include "hyper/arch/glomgr.hh"
#include "hyper/base/pardg5v.hh"
#include "hyper/gm/CPConnector.hh"
#include "mem/dram_ctrl.hh"
#include "params/PARDg5VDRAMCtrl.hh"

/**
 * The PARDg5VDRAMCtrl extend DRAM controller with guest address
 * remap ability.
 */
class PARDg5VDRAMCtrl : public DRAMCtrl, ManagedResource
{

  protected:

    CPConnector *connector;

  public:

    PARDg5VDRAMCtrl(const PARDg5VDRAMCtrlParams* p);

    virtual void init();

  public:
    /** ManagedResource Interface */
    virtual     void set(LDomID ldomID, uint32_t destAddr, uint64_t data);
    virtual uint64_t get(LDomID ldomID, uint32_t destAddr);
    // old interface
    virtual              void   set(uint64_t tag, int idx, const std::string &value) {}
    virtual const std::string query(uint64_t tag, int idx) { return "UN-IMPL"; }

};

#endif //__HYPER_MEM_PARDG5V_DRAM_CTRL_HH__
