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

#include "debug/I8254.hh"
#include "hyper/base/pardg5v.hh"
#include "hyper/dev/pardg5v/intdev.hh"
#include "hyper/dev/pardg5v/i8254p5v.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

void
X86ISA::I8254P5V::counterInterrupt(GuestID guestID, unsigned int num)
{
    DPRINTF(I8254, "Guest#%d: Interrupt from counter %d.\n", guestID, num);
    if (num == 0) {
        intPin->raise(guestID);
        //XXX This is a hack.
        intPin->lower(guestID);
    }
}

X86ISA::I8254P5V::I8254P5V(Params *p)
  : BasicPioDevice(p, 4), latency(p->pio_latency),
    intPin(p->int_pin), max_guests(p->max_guests)
{
    for (int id=0; id<max_guests; id++)
        pit.push_back(new X86Intel8254Timer(p->name, id, this));
}

X86ISA::I8254P5V::~I8254P5V()
{
    for (int id=0; id<max_guests; id++)
        delete pit[id];
}

Tick
X86ISA::I8254P5V::read(PacketPtr pkt)
{
    assert(pkt->getSize() == 1);
    Addr offset = pkt->getAddr() - pioAddr;
    GuestID guestID = PARDG5V_PKT_TO_GUESTID(pkt);
    if (offset < 3) {
        pkt->set(pit[guestID]->readCounter(offset));
    } else if (offset == 3) {
        pkt->set(uint8_t(-1));
    } else {
        panic("Read from undefined i8254 register.\n");
    }
    pkt->makeAtomicResponse();
    return latency;
}

Tick
X86ISA::I8254P5V::write(PacketPtr pkt)
{
    assert(pkt->getSize() == 1);
    Addr offset = pkt->getAddr() - pioAddr;
    GuestID guestID = PARDG5V_PKT_TO_GUESTID(pkt);
    if (offset < 3) {
        pit[guestID]->writeCounter(offset, pkt->get<uint8_t>());
    } else if (offset == 3) {
        pit[guestID]->writeControl(pkt->get<uint8_t>());
    } else {
        panic("Write to undefined i8254 register.\n");
    }
    pkt->makeAtomicResponse();
    return latency;
}

void
X86ISA::I8254P5V::serialize(std::ostream &os)
{
    for (int id=0; id<max_guests; id++)
        pit[id]->serialize(csprintf("pit%d", id), os);
}

void
X86ISA::I8254P5V::unserialize(Checkpoint *cp, const std::string &section)
{
    for (int id=0; id<max_guests; id++)
        pit[id]->unserialize(csprintf("pit%d", id), cp, section);
}

X86ISA::I8254P5V *
I8254P5VParams::create()
{
    return new X86ISA::I8254P5V(this);
}
