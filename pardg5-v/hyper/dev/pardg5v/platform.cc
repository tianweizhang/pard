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

/** @file
 * Implementation of PC platform.
 */

#include <deque>
#include <string>
#include <vector>

#include "arch/x86/intmessage.hh"
#include "arch/x86/x86_traits.hh"
#include "config/the_isa.hh"
#include "cpu/intr_control.hh"
#include "dev/x86/i8259.hh"
#include "dev/terminal.hh"
#include "hyper/dev/pardg5v/i82094aap5v.hh"
#include "hyper/dev/pardg5v/i8254p5v.hh"
#include "hyper/dev/pardg5v/platform.hh"
#include "hyper/dev/pardg5v/south_bridge.hh"

using namespace std;
using namespace TheISA;

PARDg5VPlatform::PARDg5VPlatform(const Params *p)
    : Platform(p), system(static_cast<X86PARDg5VSystem *>(p->system))
{
    southBridge = NULL;
}

void
PARDg5VPlatform::init()
{
    assert(southBridge);

    /*
     * Initialize the timer.
     */
    I8254P5V & timer = *southBridge->pit;
    for (int id=0; id<system->numPARDg5VLDomain(); id++) {
        //Timer 0, mode 2, no bcd, 16 bit count
        timer.writeControl(id, 0x34);
        //Timer 0, latch command
        timer.writeControl(id, 0x00);
        //Write a 16 bit count of 0
        timer.writeCounter(id, 0, 0);
        timer.writeCounter(id, 0, 0);
    }

    /*
     * Initialize the I/O APIC.
     */
    I82094AAP5V & ioApic = *southBridge->ioApic;
    I82094AAP5V::RedirTableEntry entry = 0;
    entry.deliveryMode = DeliveryMode::ExtInt;
    entry.vector = 0x20;
    ioApic.writeReg(0x10, entry.bottomDW);
    ioApic.writeReg(0x11, entry.topDW);
    entry.deliveryMode = DeliveryMode::Fixed;
    entry.vector = 0x24;
    ioApic.writeReg(0x18, entry.bottomDW);
    ioApic.writeReg(0x19, entry.topDW);
    entry.mask = 1;
    entry.vector = 0x21;
    ioApic.writeReg(0x12, entry.bottomDW);
    ioApic.writeReg(0x13, entry.topDW);
    entry.vector = 0x20;
    ioApic.writeReg(0x14, entry.bottomDW);
    ioApic.writeReg(0x15, entry.topDW);
    entry.vector = 0x28;
    ioApic.writeReg(0x20, entry.bottomDW);
    ioApic.writeReg(0x21, entry.topDW);
    entry.vector = 0x2C;
    ioApic.writeReg(0x28, entry.bottomDW);
    ioApic.writeReg(0x29, entry.topDW);
    entry.vector = 0x2E;
    ioApic.writeReg(0x2C, entry.bottomDW);
    ioApic.writeReg(0x2D, entry.topDW);
    entry.vector = 0x30;
    ioApic.writeReg(0x30, entry.bottomDW);
    ioApic.writeReg(0x31, entry.topDW);
}

void
PARDg5VPlatform::postConsoleInt()
{
    southBridge->ioApic->signalInterrupt(4);
}

void
PARDg5VPlatform::postConsoleInt(GuestID guestID)
{
    southBridge->ioApic->signalInterrupt(guestID, 4);
}

void
PARDg5VPlatform::clearConsoleInt()
{
    warn_once("Don't know what interrupt to clear for console.\n");
    //panic("Need implementation\n");
}

void
PARDg5VPlatform::postPciInt(int line)
{
    southBridge->ioApic->signalInterrupt(line);
}

void
PARDg5VPlatform::postPciInt(GuestID guestID, int line)
{
    southBridge->ioApic->signalInterrupt(guestID, line);
}

void
PARDg5VPlatform::clearPciInt(int line)
{
    warn_once("Tried to clear PCI interrupt %d\n", line);
}

Addr
PARDg5VPlatform::pciToDma(Addr pciAddr) const
{
    return pciAddr;
}

Addr
PARDg5VPlatform::calcPciConfigAddr(int bus, int dev, int func)
{
    assert(func < 8);
    assert(dev < 32);
    assert(bus == 0);
    return (PhysAddrPrefixPciConfig | (func << 8) | (dev << 11));
}

Addr
PARDg5VPlatform::calcPciIOAddr(Addr addr)
{
    return PhysAddrPrefixIO + addr;
}

Addr
PARDg5VPlatform::calcPciMemAddr(Addr addr)
{
    return addr;
}

PARDg5VPlatform *
PARDg5VPlatformParams::create()
{
    return new PARDg5VPlatform(this);
}
