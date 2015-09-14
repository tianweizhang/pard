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
 * Authors: Gabe Black
 *          Jiuyue Ma 
 */

/**@update
 * majiuyue: add support for PARDg5-V interrupt re-routing
 */

#ifndef __HYPER_DEV_PARDG5V_I82094AAP5V_HH__
#define __HYPER_DEV_PARDG5V_I82094AAP5V_HH__

#include <map>

#include "base/bitunion.hh"
#include "dev/io_device.hh"
#include "hyper/base/pardg5v.hh"
#include "hyper/dev/pardg5v/intdev.hh"
#include "params/I82094AAP5V.hh"

namespace X86ISA
{

class I8259;
class Interrupts;

class I82094AAP5V : public BasicPioDevice, public PARDg5VISA::IntDevice
{
  public:
    BitUnion64(RedirTableEntry)
        Bitfield<63, 32> topDW;
        Bitfield<47, 32> topReserved;
        Bitfield<31, 0> bottomDW;
        Bitfield<31, 17> bottomReserved;
        Bitfield<63, 56> dest;
        Bitfield<55, 48> guestID;
        Bitfield<16> mask;
        Bitfield<15> trigger;
        Bitfield<14> remoteIRR;
        Bitfield<13> polarity;
        Bitfield<12> deliveryStatus;
        Bitfield<11> destMode;
        Bitfield<10, 8> deliveryMode;
        Bitfield<7, 0> vector;
    EndBitUnion(RedirTableEntry)

  protected:
    I8259 * extIntPic;

    uint8_t initialApicId;
    uint8_t id;
    uint8_t arbId;
    std::vector<uint8_t> regSel;	// XXX: PARD extended

    uint64_t lowestPriorityOffset;

    // Maximum guests support
    int max_guests;

    static const uint8_t TableSize = 24;
    // This implementation is based on version 0x11, but 0x14 avoids having
    // to deal with the arbitration and APIC bus guck.
    static const uint8_t APICVersion = 0x14;

    std::vector<uint64_t> pinMask;	// XXX: PARD extended
    std::vector<RedirTableEntry *> redirTable;	// XXX: PARD extended
    bool pinStates[TableSize];

  public:
    typedef I82094AAP5VParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    I82094AAP5V(Params *p);
    virtual ~I82094AAP5V();

    void init();

    Tick read(PacketPtr pkt);
    Tick write(PacketPtr pkt);

    AddrRangeList getIntAddrRange() const;

    void writeReg(uint8_t offset, uint32_t value);
    void writeReg(GuestID guestID, uint8_t offset, uint32_t value);
    uint32_t readReg(GuestID guestID, uint8_t offset);

    BaseMasterPort &getMasterPort(const std::string &if_name,
                                  PortID idx = InvalidPortID);

    void signalInterrupt(int line);
    void raiseInterruptPin(int number);
    void lowerInterruptPin(int number);

    // PARDg5-V interrupt interface
    void signalInterrupt(GuestID guestID, int line);
    void raiseInterruptPin(GuestID guestID, int number);
    void lowerInterruptPin(GuestID guestID, int number);

    // for g++ warning: "hides overloaded virtual function [-Woverloaded-virtual]"
    using PARDg5VISA::IntDevice::raiseInterruptPin;
    using PARDg5VISA::IntDevice::lowerInterruptPin;

    virtual void serialize(std::ostream &os);
    virtual void unserialize(Checkpoint *cp, const std::string &section);
};

} // namespace X86ISA

#endif //__HYPER_DEV_PARDG5V_I82094AAP5V_HH__
