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

#include "debug/DRAM.hh"
#include "hyper/mem/dram_ctrl.hh"

using namespace std;

PARDg5VDRAMCtrl::PARDg5VDRAMCtrl(const PARDg5VDRAMCtrlParams* p)
    : DRAMCtrl(p), ManagedResource(p->name, p->agent),
      connector(p->connector)
{
    // Initial logical domain address mapping
    if (p->original_ranges.size() != p->remapped_ranges.size())
        fatal("PARDg5VDRAMCtrl: original and shadowed range list must "
              "be same size\n");

    // Initial default param table
    DPRINTFN("Initialize priority...\n");
    DPRINTFN("       | Priority | Effective | RowBufMask |\n");
    for (int i=0; i<4; i++) {
        params[i].priority = 0;
        params[i].effective_priority = 0;
        params[i].rowbuf_mask = 1;
/*
        params[i].priority = i;
        params[i].effective_priority = (i==0) ? 0 : 1;
        params[i].rowbuf_mask = (i==0) ? 0x1 : 0x2;
*/
        DPRINTFN("LDom#%d | %8d | %9d | %10X |\n", i,
                 params[i].priority, params[i].effective_priority,
                 params[i].rowbuf_mask);
    }
    DPRINTFN("\n");

    // Register managed resource
    connector->registerManagedResource(static_cast<ManagedResource *>(this));
}

void
PARDg5VDRAMCtrl::init()
{
    DRAMCtrl::init();
    _agent->mgr()->registerManagedResource(static_cast<ManagedResource *>(this));
}

union CPA_DESTADDR
{
    uint32_t data;

    struct {
        uint16_t __hidden_low : 14;
        uint8_t type : 2;
        uint16_t __hidden_high : 16;
    } hdr;

    // Address decoder for Parameter/Statistic/Trigger table
    struct {
        uint8_t offset : 8;	// offset>>8
        uint8_t __reserved : 6;
        uint8_t type : 2;
        uint16_t __hidden : 16;
    } common;

    // Address decoder for address mapping (tlb)
    struct {
        uint8_t selector : 8;
        uint8_t __reserved : 4;
        uint8_t offset : 2;
        uint8_t type : 2;
        uint16_t __hidden : 16;
    } tlb;
};

// CPA_DESTADDR::hdr::type
#define TYPE_PARAMETERS    0
#define TYPE_STATISTICS    1
#define TYPE_TRIGGERS      2
#define TYPE_TLB           3

// accessor forCPA_DESTADDR
#define CPA_DESTADDR_TYPE(destAddr)	((destAddr).hdr.type)
#define CPA_DESTADDR_FIELD(destAddr, field)         \
    ((CPA_DESTADDR_TYPE(destAddr) == TYPE_TLB) ?    \
        (destAddr).tlb.field : (destAddr).common.field)
#define CPA_DESTADDR_OFFSET(destAddr)   CPA_DESTADDR_FIELD(destAddr, offset)
#define CPA_DESTADDR_SELECTOR(destAddr) CPA_DESTADDR_FIELD(destAddr, selector)


uint64_t
PARDg5VDRAMCtrl::get(LDomID ldomID, uint32_t destAddr)
{
    CPA_DESTADDR cpaDestAddr;
    uint64_t *table_row_buffer = NULL;
    uint64_t retval;

    // TODO: how to deal with unused-ldom
    if (ldomID >= 4)
        return -1;

    cpaDestAddr.data = destAddr;
    switch (CPA_DESTADDR_TYPE(cpaDestAddr))
    {
      case TYPE_PARAMETERS:
        table_row_buffer = (uint64_t *)&params[ldomID];
        retval = table_row_buffer[CPA_DESTADDR_OFFSET(cpaDestAddr)];
        break;
      case TYPE_STATISTICS:
        table_row_buffer = stats.getStatsPtr<uint64_t>(ldomID);
        retval = table_row_buffer[CPA_DESTADDR_OFFSET(cpaDestAddr)];
        break;
      case TYPE_TRIGGERS:
        retval = 0xFFFFCCCCCCCCCCCC;
        break;
      case TYPE_TLB:
        retval = 0xFFFFDDDDDDDDDDDD;
        break;
      default:
        warn("PARDg5VDRAM: try to access non-exists table (type: %d).\n",
             CPA_DESTADDR_TYPE(cpaDestAddr));
        return -1;
    }

    return retval;
}

void
PARDg5VDRAMCtrl::set(LDomID ldomID, uint32_t destAddr, uint64_t data)
{
    CPA_DESTADDR cpaDestAddr;
    uint64_t *table_row_buffer = NULL;

    // TODO: how to deal with unused-ldom
    if (ldomID >= 4) {
        warn("%s: Try to access non-exists LDom#%d.", ldomID);
        return;
    }

    cpaDestAddr.data = destAddr;
    switch (CPA_DESTADDR_TYPE(cpaDestAddr))
    {
      case TYPE_PARAMETERS:
        table_row_buffer = (uint64_t *)&params[ldomID];
        table_row_buffer[CPA_DESTADDR_OFFSET(cpaDestAddr)] = data;
        break;
      case TYPE_TRIGGERS:
        warn("%s: Access to triggers not implement.", name());
        break;
      case TYPE_TLB:
        warn("%s: Access to TLB not implement.", name());
        break;
      case TYPE_STATISTICS:
        warn("%s: Try to access read-only statistics.", name());
        break;
      default:
        warn("%s: try to access non-exists table (type: %d).\n",
             name(), CPA_DESTADDR_TYPE(cpaDestAddr));
    }
}

PARDg5VDRAMCtrl*
PARDg5VDRAMCtrlParams::create()
{
    return new PARDg5VDRAMCtrl(this);
}

