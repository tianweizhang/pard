/*
 * Copyright (c) 2013 ACS, ICT
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
 * Declaration of top level class for PC platform components. This class
 * just retains pointers to all its children so the children can communicate.
 */

#ifndef __HYPER_DEV_PARDG5V_HH__
#define __HYPER_DEV_PARDG5V_HH__

#include "dev/platform.hh"
#include "hyper/arch/system.hh"
#include "hyper/base/pardg5v.hh"
#include "params/PARDg5VPlatform.hh"

class IdeController;
class System;
class PARDg5VSouthBridge;

class PARDg5VPlatform : public Platform
{
  public:
    /** Pointer to the system */
    X86PARDg5VSystem *system;
    PARDg5VSouthBridge *southBridge;

  public:
    typedef PARDg5VPlatformParams Params;

    /**
     * Do platform initialization stuff
     */
    void init();

    PARDg5VPlatform(const Params *p);

    /**
     * Cause the cpu to post a serial interrupt to the CPU.
     */
    virtual void postConsoleInt();
    void postConsoleInt(GuestID guestID);

    /**
     * Clear a posted CPU interrupt
     */
    virtual void clearConsoleInt();

    /**
     * Cause the chipset to post a pci interrupt to the CPU.
     */
    virtual void postPciInt(int line);
    void postPciInt(GuestID guestID, int line);

    /**
     * Clear a posted PCI->CPU interrupt
     */
    virtual void clearPciInt(int line);


    virtual Addr pciToDma(Addr pciAddr) const;

    /**
     * Calculate the configuration address given a bus/dev/func.
     */
    virtual Addr calcPciConfigAddr(int bus, int dev, int func);

    /**
     * Calculate the address for an IO location on the PCI bus.
     */
    virtual Addr calcPciIOAddr(Addr addr);

    /**
     * Calculate the address for a memory location on the PCI bus.
     */
    virtual Addr calcPciMemAddr(Addr addr);
};

#endif // __DEV_PC_HH__
