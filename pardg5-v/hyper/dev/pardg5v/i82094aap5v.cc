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

#include "arch/x86/interrupts.hh"
#include "arch/x86/intmessage.hh"
#include "cpu/base.hh"
#include "debug/I82094AAP5V.hh"
#include "dev/x86/i8259.hh"
#include "hyper/dev/pardg5v/i82094aap5v.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/system.hh"

X86ISA::I82094AAP5V::I82094AAP5V(Params *p)
    : BasicPioDevice(p, 20), PARDg5VISA::IntDevice(this, p->int_latency),
      extIntPic(p->external_int_pic), lowestPriorityOffset(0)
{
    // This assumes there's only one I/O APIC in the system and since the apic
    // id is stored in a 8-bit field with 0xff meaning broadcast, the id must
    // be less than 0xff

    assert(p->apic_id < 0xff);
    initialApicId = id = p->apic_id;
    arbId = id;
    
    // support maximum 255 guests, limited by APIC-ID
    assert(p->max_guests < 0xFF);
    max_guests = p->max_guests;

    // RegSel register
    while (regSel.size() < p->max_guests)
        regSel.push_back(0);

    // PinMask must less/equal than max_guests
    assert(p->pin_mask.size() <= p->max_guests);
    // assign configed PinMask
    pinMask = p->pin_mask;
    // assign unconfiged PinMask to 0
    while (pinMask.size() < p->max_guests)
        pinMask.push_back(0);

    // Interrupt Redirect Table
    redirTable.resize(max_guests);
    for (int id=0; id<max_guests; id++) {
        redirTable[id] = new RedirTableEntry[TableSize];
        RedirTableEntry entry = 0;
        entry.mask = 1;
        entry.guestID = id;
        for (int i=0; i<TableSize; i++)
            redirTable[id][i] = entry;
    }
    for (int i = 0; i < TableSize; i++)
        pinStates[i] = false;
}

X86ISA::I82094AAP5V::~I82094AAP5V()
{
    for (int id=0; id<max_guests; id++)
        delete[] redirTable[id];
}

void
X86ISA::I82094AAP5V::init()
{
    // The io apic must register its address ranges on both its pio port
    // via the piodevice init() function and its int port that it inherited
    // from IntDevice.  Note IntDevice is not a SimObject itself.

    BasicPioDevice::init();
    PARDg5VISA::IntDevice::init();
}

BaseMasterPort &
X86ISA::I82094AAP5V::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "int_master")
        return intMasterPort;
    return BasicPioDevice::getMasterPort(if_name, idx);
}

AddrRangeList
X86ISA::I82094AAP5V::getIntAddrRange() const
{
    AddrRangeList ranges;
    ranges.push_back(RangeEx(x86InterruptAddress(initialApicId, 0),
                             x86InterruptAddress(initialApicId, 0) +
                             PhysAddrAPICRangeSize));
    return ranges;
}

Tick
X86ISA::I82094AAP5V::read(PacketPtr pkt)
{
    assert(pkt->getSize() == 4);
    Addr offset = pkt->getAddr() - pioAddr;
    GuestID guestID = PARDG5V_PKT_TO_GUESTID(pkt);
    switch(offset) {
      case 0:
        pkt->set<uint32_t>(regSel[guestID]);
        break;
      case 16:
        pkt->set<uint32_t>(readReg(guestID, regSel[guestID]));
        break;
      default:
        panic("Illegal read from I/O APIC.\n");
    }
    pkt->makeAtomicResponse();
    return pioDelay;
}

Tick
X86ISA::I82094AAP5V::write(PacketPtr pkt)
{
    assert(pkt->getSize() == 4);
    Addr offset = pkt->getAddr() - pioAddr;
    GuestID guestID = PARDG5V_PKT_TO_GUESTID(pkt);
    switch(offset) {
      case 0:
        regSel[guestID] = pkt->get<uint32_t>();
        break;
      case 16:
        writeReg(guestID, regSel[guestID], pkt->get<uint32_t>());
        break;
      default:
        panic("Illegal write to I/O APIC.\n");
    }
    pkt->makeAtomicResponse();
    return pioDelay;
}

void
X86ISA::I82094AAP5V::writeReg(uint8_t offset, uint32_t value)
{
    GuestID guestID;
    for (guestID=0; guestID<max_guests; guestID++)
        writeReg(guestID, offset, value);
}

void
X86ISA::I82094AAP5V::writeReg(GuestID guestID, uint8_t offset, uint32_t value)
{
    if (offset == 0x0) {
        id = bits(value, 31, 24);
    } else if (offset == 0x1) {
        // The IOAPICVER register is read only.
    } else if (offset == 0x2) {
        arbId = bits(value, 31, 24);
    } else if (offset >= 0x10 && offset <= (0x10 + TableSize * 2)) {
        int index = (offset - 0x10) / 2;
        if (offset % 2) {
            redirTable[guestID][index].topDW = value;
            redirTable[guestID][index].topReserved = 0;
        } else {
            redirTable[guestID][index].bottomDW = value;
            redirTable[guestID][index].bottomReserved = 0;
        }
    } else {
        warn("Guest#%d: Access to undefined I/O APIC register %#x.\n",
             guestID, offset);
    }
    DPRINTF(I82094AAP5V,
            "Guest#%d: Wrote %#x to I/O APIC register %#x .\n",
            guestID, value, offset);
}

uint32_t
X86ISA::I82094AAP5V::readReg(GuestID guestID, uint8_t offset)
{
    uint32_t result = 0;
    if (offset == 0x0) {
        result = id << 24;
    } else if (offset == 0x1) {
        result = ((TableSize - 1) << 16) | APICVersion;
    } else if (offset == 0x2) {
        result = arbId << 24;
    } else if (offset >= 0x10 && offset <= (0x10 + TableSize * 2)) {
        int index = (offset - 0x10) / 2;
        if (offset % 2) {
            result = redirTable[guestID][index].topDW;
        } else {
            result = redirTable[guestID][index].bottomDW;
        }
    } else {
        warn("Guest#%d: Access to undefined I/O APIC register %#x.\n",
             guestID, offset);
    }
    DPRINTF(I82094AAP5V,
            "Guest#%d: Read %#x from I/O APIC register %#x.\n",
            guestID, result, offset);
    return result;
}

void
X86ISA::I82094AAP5V::signalInterrupt(int line)
{
    // Signal all guest, for old-device compatible.
    for (GuestID guestID=0; guestID<max_guests; guestID++) {
        if (!(pinMask[guestID] & 1<<line))
            continue;
        signalInterrupt(guestID, line);
    }
}

void
X86ISA::I82094AAP5V::signalInterrupt(GuestID guestID, int line)
{
    DPRINTF(I82094AAP5V, "Guest#%d: Received interrupt %d.\n", guestID, line);
    assert(guestID < max_guests);
    assert(line < TableSize);

    if (!(pinMask[guestID] & 1<<line)) {
        DPRINTF(I82094AAP5V, "Guest#%d: Entry %d not allowed.\n", guestID, line);
        return;
    }

    RedirTableEntry entry = redirTable[guestID][line];
    if (entry.mask) {
        DPRINTF(I82094AAP5V, "Guest#%d: Entry was masked.\n", guestID);
        return;
    } else {
        TriggerIntMessage message = 0;
        message.destination = entry.dest;
        if (entry.deliveryMode == DeliveryMode::ExtInt) {
            warn("ExtInt delivery mode not support by I82094AAP5V.\n");
            return;
            assert(extIntPic);
            message.vector = extIntPic->getVector();
        } else {
            message.vector = entry.vector;
        }
        message.deliveryMode = entry.deliveryMode;
        message.destMode = entry.destMode;
        message.level = entry.polarity;
        message.trigger = entry.trigger;
        ApicList apics;
        int numContexts = sys->numContexts();
        if (message.destMode == 0) {
            if (message.deliveryMode == DeliveryMode::LowestPriority) {
                panic("Lowest priority delivery mode from the "
                        "IO APIC aren't supported in physical "
                        "destination mode.\n");
            }
            if (message.destination == 0xFF) {
                for (int i = 0; i < numContexts; i++) {
                    apics.push_back(i);
                }
            } else {
                apics.push_back(message.destination);
            }
        } else {
            for (int i = 0; i < numContexts; i++) {
                // Check guestID
                if ((sys->getThreadContext(i)->readMiscRegNoEffect(MISCREG_QR0) & 0xFF) != guestID)
                    continue;

                Interrupts *localApic = sys->getThreadContext(i)->
                    getCpuPtr()->getInterruptController();
                
                if ((localApic->readReg(APIC_LOGICAL_DESTINATION) >> 24) &
                        message.destination) {
                    apics.push_back(localApic->getInitialApicId());
                }
            }
            if (message.deliveryMode == DeliveryMode::LowestPriority &&
                    apics.size()) {
                // The manual seems to suggest that the chipset just does
                // something reasonable for these instead of actually using
                // state from the local APIC. We'll just rotate an offset
                // through the set of APICs selected above.
                uint64_t modOffset = lowestPriorityOffset % apics.size();
                lowestPriorityOffset++;
                ApicList::iterator apicIt = apics.begin();
                while (modOffset--) {
                    apicIt++;
                    assert(apicIt != apics.end());
                }
                int selected = *apicIt;
                apics.clear();
                apics.push_back(selected);
            }
        }
        intMasterPort.sendMessage(apics, message, sys->isTimingMode());
    }
}

void
X86ISA::I82094AAP5V::raiseInterruptPin(int number)
{
    assert(number < TableSize);
    if (!pinStates[number])
        signalInterrupt(number);
    pinStates[number] = true;
}

void
X86ISA::I82094AAP5V::raiseInterruptPin(GuestID guest, int number)
{
    assert(number < TableSize);
    // XXX, BUGS here: if different guest raise same pin, pinStates will be wrong
    if (!pinStates[number])
        signalInterrupt(guest, number);
    pinStates[number] = true;
}

void
X86ISA::I82094AAP5V::lowerInterruptPin(GuestID guest, int number)
{
    assert(number < TableSize);
    pinStates[number] = false;
}

void
X86ISA::I82094AAP5V::lowerInterruptPin(int number)
{
    assert(number < TableSize);
    pinStates[number] = false;
}

#define SERIALIZE_INDEXED_ARRAY(member, idx, size) {   \
    char __buf[256];                                   \
    snprintf(__buf, 255, #member"%d", idx);            \
    arrayParamOut(os, __buf, member, size);            \
}

#define UNSERIALIZE_INDEXED_ARRAY(member, idx, size) { \
    char __buf[256];                                   \
    snprintf(__buf, 255, #member"%d", idx);            \
    arrayParamIn(cp, section, __buf, member, size);    \
}

#define   SERIALIZE_VECTOR(member)  arrayParamOut(os, #member, member)
#define UNSERIALIZE_VECTOR(member)  arrayParamIn (cp, section, #member, member)

void
X86ISA::I82094AAP5V::serialize(std::ostream &os)
{
    SERIALIZE_VECTOR(regSel);
    SERIALIZE_SCALAR(initialApicId);
    SERIALIZE_SCALAR(id);
    SERIALIZE_SCALAR(arbId);
    SERIALIZE_SCALAR(lowestPriorityOffset);
    SERIALIZE_SCALAR(max_guests);
    SERIALIZE_VECTOR(pinMask);
    for (int i=0; i<max_guests; i++) {
        uint64_t* redirTableArray = (uint64_t*)redirTable[i];
        SERIALIZE_INDEXED_ARRAY(redirTableArray, i, TableSize);
    }
    SERIALIZE_ARRAY(pinStates, TableSize);
}

void
X86ISA::I82094AAP5V::unserialize(Checkpoint *cp, const std::string &section)
{
    uint64_t redirTableArray[TableSize];
    UNSERIALIZE_VECTOR(regSel);
    UNSERIALIZE_SCALAR(initialApicId);
    UNSERIALIZE_SCALAR(id);
    UNSERIALIZE_SCALAR(arbId);
    UNSERIALIZE_SCALAR(lowestPriorityOffset);
    UNSERIALIZE_SCALAR(max_guests);
    UNSERIALIZE_VECTOR(pinMask);
    for (int id=0; id < max_guests; id++) {
        UNSERIALIZE_INDEXED_ARRAY(redirTableArray, id, TableSize);
        for (int i = 0; i < TableSize; i++) {
            redirTable[id][i] = (RedirTableEntry)redirTableArray[i];
        }
    }
    UNSERIALIZE_ARRAY(pinStates, TableSize);
}

X86ISA::I82094AAP5V *
I82094AAP5VParams::create()
{
    return new X86ISA::I82094AAP5V(this);
}
