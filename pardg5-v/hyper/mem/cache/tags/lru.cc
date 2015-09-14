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
 * Copyright (c) 2012-2013 ARM Limited
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
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 * Authors: Erik Hallnor
 *          Jiuyue Ma
 */

/**
 * @file
 * Definitions of LRU tag store.
 */

#include <string>

#include "base/cprintf.hh"
#include "base/intmath.hh"
#include "debug/Cache.hh"
#include "debug/CacheRepl.hh"
#include "hyper/arch/system.hh"
#include "hyper/base/pardg5v.hh"
#include "hyper/mem/cache/tags/lru.hh"
#include "hyper/mem/cache/base.hh"
#include "sim/core.hh"

using namespace std;

namespace PARDg5VISA {

LRU::LRU(const Params *p)
    :BaseTags(p), ManagedResource(p->name, p->agent),
     connector(p->connector.empty() ? NULL : p->connector[0]), 
     assoc(p->assoc),
     numSets(p->size / (p->block_size * p->assoc)),
     sequentialAccess(p->sequential_access),
     waymasks(p->waymasks)
{
    // Check parameters
    if (blkSize < 4 || !isPowerOf2(blkSize)) {
        fatal("Block size must be at least 4 and a power of 2");
    }
    if (numSets <= 0 || !isPowerOf2(numSets)) {
        fatal("# of sets must be non-zero and a power of 2");
    }
    if (assoc <= 0) {
        fatal("associativity must be greater than zero");
    }
    if (hitLatency <= 0) {
        fatal("access latency must be greater than zero");
    }

    blkMask = blkSize - 1;
    setShift = floorLog2(blkSize);
    setMask = numSets - 1;
    tagShift = setShift + floorLog2(numSets);
    warmedUp = false;
    /** @todo Make warmup percentage a parameter. */
    warmupBound = numSets * assoc;

    sets = new SetType[numSets];
    blks = new BlkType[numSets * assoc];
    // allocate data storage in one big chunk
    numBlocks = numSets * assoc;
    dataBlks = new uint8_t[numBlocks * blkSize];

    unsigned blkIndex = 0;       // index into blks array
    for (unsigned i = 0; i < numSets; ++i) {
        sets[i].assoc = assoc;

        sets[i].blks = new BlkType*[assoc];

        // link in the data blocks
        for (unsigned j = 0; j < assoc; ++j) {
            // locate next cache block
            BlkType *blk = &blks[blkIndex];
            blk->data = &dataBlks[blkSize*blkIndex];
            ++blkIndex;

            // invalidate new cache block
            blk->invalidate();

            //EGH Fix Me : do we need to initialize blk?

            // Setting the tag to j is just to prevent long chains in the hash
            // table; won't matter because the block is invalid
            blk->tag = j;
            blk->whenReady = 0;
            blk->isTouched = false;
            blk->size = blkSize;
            sets[i].blks[j]=blk;
            blk->set = i;
            blk->way = j;
        }
    }
}

LRU::~LRU()
{
    delete [] dataBlks;
    delete [] blks;
    delete [] sets;
}

void
LRU::init()
{
    BaseTags::init();
    _agent->mgr()->registerManagedResource(static_cast<ManagedResource *>(this));
    if (connector)
        connector->registerManagedResource(static_cast<ManagedResource *>(this));
}

LRU::BlkType*
LRU::accessBlock(GuestID guest, Addr addr, bool is_secure, Cycles &lat, int master_id)
{
    Addr tag = extractTag(addr);
    unsigned set = extractSet(addr);
    BlkType *blk = sets[set].findBlk(guest, tag, is_secure);
    lat = hitLatency;

    // Access all tags in parallel, hence one in each way.  The data side
    // either accesses all blocks in parallel, or one block sequentially on
    // a hit.  Sequential access with a miss doesn't access data.
    tagAccesses += assoc;
    if (sequentialAccess) {
        if (blk != NULL) {
            dataAccesses += 1;
        }
    } else {
        dataAccesses += assoc;
    }

    if (blk != NULL) {
        // move this block to head of the MRU list
        sets[set].moveToHead(blk);
        DPRINTF(CacheRepl, "set %x: moving blk [Guest #%d] %x (%s) to MRU\n",
                set, guest, regenerateBlkAddr(tag, set), is_secure ? "s" : "ns");
        if (blk->whenReady > curTick()
            && cache->ticksToCycles(blk->whenReady - curTick()) > hitLatency) {
            lat = cache->ticksToCycles(blk->whenReady - curTick());
        }
        blk->refCount += 1;
    }

    return blk;
}


LRU::BlkType*
LRU::findBlock(GuestID guest, Addr addr, bool is_secure) const
{
    Addr tag = extractTag(addr);
    unsigned set = extractSet(addr);
    BlkType *blk = sets[set].findBlk(guest, tag, is_secure);
    return blk;
}

LRU::BlkType*
LRU::findVictim(GuestID guest, Addr addr)
{
    unsigned set = extractSet(addr);
    BlkType *blk = NULL;
    uint64_t waymask = waymasks[guest];
    assert(waymask);

    // grab a replacement candidate
    for (int i=assoc-1; i>=0; i--) {
        blk = sets[set].blks[i];
        if ((1<<blk->way) & waymask)
            break;
    }

    if (blk->isValid()) {
        DPRINTF(CacheRepl, "set %x: Guest #%d selecting blk %x [Guest #%d] for replacement\n",
                set, guest, regenerateBlkAddr(blk->tag, set), blk->getGuestID());
    }
    return blk;
}

void
LRU::insertBlock(GuestID guest, PacketPtr pkt, BlkType *blk)
{
    Addr addr = pkt->getAddr();
    MasterID master_id = pkt->req->masterId();
    uint32_t task_id = pkt->req->taskId();
    bool is_secure = pkt->isSecure();
    if (!blk->isTouched) {
        tagsInUse++;
        blk->isTouched = true;
        if (!warmedUp && tagsInUse.value() >= warmupBound) {
            warmedUp = true;
            warmupCycle = curTick();
        }
    }

    // If we're replacing a block that was previously valid update
    // stats for it. This can't be done in findBlock() because a
    // found block might not actually be replaced there if the
    // coherence protocol says it can't be.
    if (blk->isValid()) {
        replacements[0]++;
        totalRefs += blk->refCount;
        ++sampledRefs;
        blk->refCount = 0;

        // deal with evicted block
        assert(blk->srcMasterId < cache->system->maxMasters());
        occupancies[blk->srcMasterId]--;

        blk->invalidate();
    }

    blk->isTouched = true;
    // Set tag for new block.  Caller is responsible for setting status.
    blk->setGuestID(guest);
    blk->tag = extractTag(addr);
    if (is_secure)
        blk->status |= BlkSecure;

    // deal with what we are bringing in
    assert(master_id < cache->system->maxMasters());
    occupancies[master_id]++;
    blk->srcMasterId = master_id;
    blk->task_id = task_id;
    blk->tickInserted = curTick();

    unsigned set = extractSet(addr);
    sets[set].moveToHead(blk);

    // We only need to write into one tag and one data block.
    tagAccesses += 1;
    dataAccesses += 1;
}

void
LRU::invalidate(BlkType *blk)
{
    assert(blk);
    assert(blk->isValid());
    tagsInUse--;
    assert(blk->srcMasterId < cache->system->maxMasters());
    occupancies[blk->srcMasterId]--;
    blk->srcMasterId = Request::invldMasterId;
    blk->task_id = ContextSwitchTaskId::Unknown;
    blk->tickInserted = curTick();
    blk->setGuestID(ALL_GUEST);

    // should be evicted before valid blocks
    unsigned set = blk->set;
    sets[set].moveToTail(blk);
}

void
LRU::clearLocks()
{
    for (int i = 0; i < numBlocks; i++){
        blks[i].clearLoadLocks();
    }
}

std::string 
LRU::print() const {
    std::string cache_state;
    for (unsigned i = 0; i < numSets; ++i) {
        // link in the data blocks
        for (unsigned j = 0; j < assoc; ++j) {
            BlkType *blk = sets[i].blks[j];
            if (blk->isValid())
                cache_state += csprintf("\tset: %d block: %d %s\n", i, j,
                        blk->print());
        }
    }
    if (cache_state.empty())
        cache_state = "no valid tags\n";
    return cache_state;
}

void
LRU::cleanupRefs()
{
    for (unsigned i = 0; i < numSets*assoc; ++i) {
        if (blks[i].isValid()) {
            totalRefs += blks[i].refCount;
            ++sampledRefs;
        }
    }
}

void
LRU::computeStats()
{
    for (unsigned i = 0; i < ContextSwitchTaskId::NumTaskId; ++i) {
        occupanciesTaskId[i] = 0;
        for (unsigned j = 0; j < 5; ++j) {
            ageTaskId[i][j] = 0;
        }
    }

    for (unsigned i = 0; i < numSets * assoc; ++i) {
        if (blks[i].isValid()) {
            assert(blks[i].task_id < ContextSwitchTaskId::NumTaskId);
            occupanciesTaskId[blks[i].task_id]++;
            Tick age = curTick() - blks[i].tickInserted;
            assert(age >= 0);

            int age_index;
            if (age / SimClock::Int::us < 10) { // <10us
                age_index = 0;
            } else if (age / SimClock::Int::us < 100) { // <100us
                age_index = 1;
            } else if (age / SimClock::Int::ms < 1) { // <1ms
                age_index = 2;
            } else if (age / SimClock::Int::ms < 10) { // <10ms
                age_index = 3;
            } else
                age_index = 4; // >10ms

            ageTaskId[blks[i].task_id][age_index]++;
            guestCapacity[blks[i].getGuestID()] ++;
        }
    }
}

/** PARDManagedResource Interface. */
void
LRU::set(uint64_t tag, int idx, const std::string &value)
{
    GuestID guestID = QOSTAG_TO_GUESTID(tag);
    uint64_t mask = atol(value.c_str());
    if (idx == 0)
        waymasks[guestID] = mask;
    else
        warn("invalid cache tag program request: <0x%x, %d, 0x%x>.", tag, idx, mask);
}

const std::string
LRU::query(uint64_t tag, int idx)
{
    GuestID guestID = QOSTAG_TO_GUESTID(tag);
    if(idx != 0) {
        warn("invalid cache tag qurey request: <0x%x, %d>.", tag, idx);
        return "INV";
    }
    return csprintf("0x%x", waymasks[guestID]);
}

void
LRU::set(LDomID ldomID, uint32_t destAddr, uint64_t data)
{
    if (destAddr == 0)
        waymasks[ldomID] = data;
    else
        warn("invalid cache tag program request: <%d, 0x%x, 0x%llu>.",
              ldomID, destAddr, data);
}

uint64_t
LRU::get(LDomID ldomID, uint32_t destAddr)
{
    uint64_t retval;

    switch (destAddr) {
      // waymask
      case 0:
        retval = (ldomID >= waymasks.size()) ? -1 : waymasks[ldomID];
        break;

      // capacity
      case 8:
        computeStats();
        retval = (ldomID >= guestCapacity.size()) ? -1
                                                  : guestCapacity[ldomID].value();
        DPRINTFN("LRU: %d => %d\n", ldomID, retval);
        break;

      // hit_count
      case 16:
        retval = 44;
        break;
 
      // miss_count
      case 24:
        retval = 88;
        break;

      default:
        warn("invalid cache tag qurey request: <%d, 0x%x>.", ldomID, destAddr);
        retval = 0;
    }

    return retval;
}


} // namespace PARDg5VISA

PARDg5VISA::LRU *
PARDg5VLRUParams::create()
{
    return new PARDg5VISA::LRU(this);
}
