/*
 * Copyright (c) 2014 ACS, ICT
 * All rights reserved
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
 * Copyright (c) 2012 ARM Limited
 * All rights reserved
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
 * Authors: Andreas Hansson
 *          Jiuyue Ma
 */

#include <string>

#include "debug/AddrMapper.hh"
#include "hyper/arch/system.hh"
#include "hyper/base/guest_addr_range.hh"
#include "hyper/mem/addr_mapper.hh"
#include "sim/system.hh"
#include "sim/stats.hh"

PARDg5VAddrMapper::PARDg5VAddrMapper(const PARDg5VAddrMapperParams* p)
    : MemObject(p), ManagedResource(p->name, p->agent),
      connector(p->connector),
      masterPort(name() + "-master", *this),
      slavePort(name() + "-slave", *this)
{
    connector->registerManagedResource(static_cast<ManagedResource *>(this));
}

void
PARDg5VAddrMapper::init()
{
    if (!slavePort.isConnected() || !masterPort.isConnected())
        fatal("Address mapper is not connected on both sides.\n");
    _agent->mgr()->registerManagedResource(static_cast<ManagedResource *>(this));
}

BaseMasterPort&
PARDg5VAddrMapper::getMasterPort(const std::string& if_name, PortID idx)
{
    if (if_name == "master") {
        return masterPort;
    } else {
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort&
PARDg5VAddrMapper::getSlavePort(const std::string& if_name, PortID idx)
{
    if (if_name == "slave") {
        return slavePort;
    } else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

void
PARDg5VAddrMapper::recvFunctional(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    pkt->setAddr(remapAddr(orig_addr, PARDG5V_PKT_TO_GUESTID(pkt)));
    masterPort.sendFunctional(pkt);
    pkt->setAddr(orig_addr);
}

void
PARDg5VAddrMapper::recvFunctionalSnoop(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    pkt->setAddr(remapAddr(orig_addr, PARDG5V_PKT_TO_GUESTID(pkt)));
    slavePort.sendFunctionalSnoop(pkt);
    pkt->setAddr(orig_addr);
}

Tick
PARDg5VAddrMapper::recvAtomic(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();

    if (pkt->isRead())
        bytesRead[PARDG5V_PKT_TO_GUESTID(pkt)] += pkt->getSize();
    if (pkt->isWrite())
        bytesWritten[PARDG5V_PKT_TO_GUESTID(pkt)] += pkt->getSize();

    pkt->setAddr(remapAddr(orig_addr, PARDG5V_PKT_TO_GUESTID(pkt)));
    DPRINTF(AddrMapper, "map (%d)0x%x => 0x%x\n", PARDG5V_PKT_TO_GUESTID(pkt), orig_addr, pkt->getAddr());
    Tick ret_tick =  masterPort.sendAtomic(pkt);
    pkt->setAddr(orig_addr);

    return ret_tick;
}

Tick
PARDg5VAddrMapper::recvAtomicSnoop(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    pkt->setAddr(remapAddr(orig_addr, PARDG5V_PKT_TO_GUESTID(pkt)));
    Tick ret_tick = slavePort.sendAtomicSnoop(pkt);
    pkt->setAddr(orig_addr);
    return ret_tick;
}

bool
PARDg5VAddrMapper::recvTimingReq(PacketPtr pkt)
{
    /// @todo how to deal with address mapping using addr_mapper
    /// NoncoherentBus can not accept ExpressSnoop, we filter all
    // InhibitAsserted request
    for (int x = 0; x < pendingDelete.size(); x++)
        delete pendingDelete[x];
    pendingDelete.clear();
    if (pkt->memInhibitAsserted()) {
        DPRINTF(AddrMapper, "Early drop inhibited packet in PARDg5VAddrMapper: addr %lld\n",
                 pkt->getAddr());
        pendingDelete.push_back(pkt);
        return true;
    }
    assert (!pkt->isExpressSnoop());
    // --------------- END of HOOK ---------------

    Addr orig_addr = pkt->getAddr();
    bool needsResponse = pkt->needsResponse();
    bool memInhibitAsserted = pkt->memInhibitAsserted();

    if (pkt->isRead())
        bytesRead[PARDG5V_PKT_TO_GUESTID(pkt)] += pkt->getSize();
    if (pkt->isWrite())
        bytesWritten[PARDG5V_PKT_TO_GUESTID(pkt)] += pkt->getSize();


    if (needsResponse && !memInhibitAsserted) {
        pkt->pushSenderState(new AddrMapperSenderState(orig_addr));
    }

    pkt->setAddr(remapAddr(orig_addr, PARDG5V_PKT_TO_GUESTID(pkt)));

    // @todo someone should pay for this
    pkt->busFirstWordDelay = pkt->busLastWordDelay = 0;

    // Attempt to send the packet (always succeeds for inhibited
    // packets)
    bool successful = masterPort.sendTimingReq(pkt);

    // If not successful, restore the sender state
    if (!successful && needsResponse) {
        delete pkt->popSenderState();
    }

    return successful;
}

bool
PARDg5VAddrMapper::recvTimingResp(PacketPtr pkt)
{
    AddrMapperSenderState* receivedState =
        dynamic_cast<AddrMapperSenderState*>(pkt->senderState);

    // Restore initial sender state
    if (receivedState == NULL)
        panic("PARDg5VAddrMapper %s got a response without sender state\n",
              name());

    Addr remapped_addr = pkt->getAddr();

    // Restore the state and address
    pkt->senderState = receivedState->predecessor;
    pkt->setAddr(receivedState->origAddr);

    // @todo someone should pay for this
    pkt->busFirstWordDelay = pkt->busLastWordDelay = 0;

    // Attempt to send the packet
    bool successful = slavePort.sendTimingResp(pkt);

    // If packet successfully sent, delete the sender state, otherwise
    // restore state
    if (successful) {
        delete receivedState;
    } else {
        // Don't delete anything and let the packet look like we did
        // not touch it
        pkt->senderState = receivedState;
        pkt->setAddr(remapped_addr);
    }

    return successful;
}

void
PARDg5VAddrMapper::recvTimingSnoopReq(PacketPtr pkt)
{
    slavePort.sendTimingSnoopReq(pkt);
}

bool
PARDg5VAddrMapper::recvTimingSnoopResp(PacketPtr pkt)
{
    return masterPort.sendTimingSnoopResp(pkt);
}

bool
PARDg5VAddrMapper::isSnooping() const
{
    if (slavePort.isSnooping())
        fatal("PARDg5VAddrMapper doesn't support remapping of snooping requests\n");
    return false;
}

void
PARDg5VAddrMapper::recvRetryMaster()
{
    slavePort.sendRetry();
}

void
PARDg5VAddrMapper::recvRetrySlave()
{
    masterPort.sendRetry();
}

void
PARDg5VAddrMapper::recvRangeChange()
{
    slavePort.sendRangeChange();
}

void
PARDg5VAddrMapper::regStats()
{
    using namespace Stats;

    int numGuests = 4;

    bytesRead
        .init(numGuests)
        .name(name() + ".bytes_read")
        .desc("Number of bytes read from this memory")
        .flags(total)
        ;
    for (int i = 0; i < numGuests; i++) {
        bytesRead.subname(i, csprintf("Guest#%d", i));
    }

    bytesWritten
        .init(numGuests)
        .name(name() + ".bytes_written")
        .desc("Number of bytes written to this memory")
        .flags(total)
        ;
    for (int i = 0; i < numGuests; i++) {
        bytesWritten.subname(i, csprintf("Guest#%d", i));
    }

    bwTotal
        .name(name() + ".bw_total")
        .desc("Total bandwidth to/from this memory (bytes/s)")
        .precision(0)
        .prereq(bwTotal)
        .flags(total)
        ;
    for (int i = 0; i < numGuests; i++) {
        bwTotal.subname(i, csprintf("Guest#%d", i));
    }

    //bwRead = bytesRead / simSeconds;
    //bwWrite = bytesWritten / simSeconds;
    bwTotal = (bytesRead + bytesWritten) / simSeconds;
}


PARDg5VRangeAddrMapper::PARDg5VRangeAddrMapper(const PARDg5VRangeAddrMapperParams* p)
    : PARDg5VAddrMapper(p)
{
    if (p->original_ranges.size() != p->remapped_ranges.size())
        fatal("PARDg5VAddrMapper: original and shadowed range list must "
              "be same size\n");

    DPRINTF(AddrMapper, "Initialize logical domain address mapping...\n");
    for (size_t x = 0; x < p->original_ranges.size(); x++) {
        if (p->original_ranges[x].size() != p->remapped_ranges[x].size())
            fatal("PARDg5VAddrMapper: original and shadowed range list elements"
                  " aren't all of the same size\n");
        tlb.push_back(LDomAddrTLBEntry(p->original_ranges[x], p->remapped_ranges[x]));
        DPRINTF(AddrMapper, p->original_ranges[x].start() & 0x8000000000000000 ? 
                     "LDom#%d:  [0x%04X, 0x%04X] => [0x%04X, 0x%04X] (I/O)\n"
                   : "LDom#%d:  [0x%08X, 0x%08X] => [0x%08X, 0x%08X]\n",

                 p->original_ranges[x].guestID(),
                 (p->original_ranges[x].start()) & 0x7FFFFFFFFFFFFFFF,
                 (p->original_ranges[x].start() + p->original_ranges[x].size() - 1) & 0x7FFFFFFFFFFFFFFF,
                 (p->remapped_ranges[x].start()) & 0x7FFFFFFFFFFFFFFF,
                 (p->remapped_ranges[x].start() + p->remapped_ranges[x].size() - 1) & 0x7FFFFFFFFFFFFFFF);
    }
    DPRINTF(AddrMapper, "\n");

/*
    for (size_t x = 0; x < p->original_ranges.size(); x++) {
        if (p->original_ranges[x].size() != p->remapped_ranges[x].size())
            fatal("PARDg5VAddrMapper: original and shadowed range list elements"
                  " aren't all of the same size\n");
        tlb.push_back(LDomAddrTLBEntry(p->original_ranges[x], p->remapped_ranges[x]));
    }
*/
}

void
PARDg5VRangeAddrMapper::set(LDomID ldomID, uint32_t destAddr, uint64_t data)
{
}

uint64_t
PARDg5VRangeAddrMapper::get(LDomID ldomID, uint32_t destAddr)
{
    return 0xFFFF0000FFFF0000;
}

void
PARDg5VRangeAddrMapper::set(uint64_t tag, int idx, const std::string &value)
{
    DPRINTFN("%s: set <0x%x, %d> => %s\n", name(), tag, idx, value);
}

const std::string
PARDg5VRangeAddrMapper::query(uint64_t tag, int idx)
{
    std::stringstream result;
    LDomID domid = QOSTAG_TO_GUESTID(tag);
    std::vector<LDomAddrTLBEntry> xents;

    if(idx != 0) {
        warn("invalid addr_mapper qurey request: <0x%x, %d>.", tag, idx);
        return "INV";
    }

    tlb.query(domid, xents);
    for (int x = 0; x < xents.size(); ++x) {
        result<<xents[x]<<std::endl;
    }

    return result.str();
}

Addr
PARDg5VRangeAddrMapper::remapAddr(Addr addr, GuestID guestID) const
{
    return tlb.remapAddr(guestID, addr);
}

AddrRangeList
PARDg5VRangeAddrMapper::getAddrRanges() const
{
    // Simply return the original ranges as given by the parameters
    //AddrRangeList ranges(originalRanges.begin(), originalRanges.end());
    //AddrRangeList ranges;
    return masterPort.getAddrRanges();
}

PARDg5VRangeAddrMapper*
PARDg5VRangeAddrMapperParams::create()
{
    return new PARDg5VRangeAddrMapper(this);
}

