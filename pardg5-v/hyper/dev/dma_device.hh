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
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
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
 * Authors: Ali Saidi
 *          Nathan Binkert
 *          Jiuyue Ma
 */

/* @update
 * majiuyue: Extend DmaDevice with PARDg5-V GuestID support.
 */

#ifndef __HYPER_DEV_DMA_DEVICE_HH__
#define __HYPER_DEV_DMA_DEVICE_HH__

#include <deque>

#include "dev/disk_image.hh"
#include "dev/io_device.hh"
#include "hyper/base/pardg5v.hh"
#include "params/PARDg5VDmaDevice.hh"
#include "sim/drain.hh"
#include "sim/system.hh"

#define GUESTNUM 2
#define BEGIN_TIME 1200000000000
#define TIME_WINDOW 1000000000

class PARDg5VDmaPort : public MasterPort
{
  private:

    /**
     * Take the first packet of the transmit list and attempt to send
     * it as a timing request. If it is successful, schedule the
     * sending of the next packet, otherwise remember that we are
     * waiting for a retry.
     */
    void trySendTimingReq();

    /**
     * For timing, attempt to send the first item on the transmit
     * list, and if it is successful and there are more packets
     * waiting, then schedule the sending of the next packet. For
     * atomic, simply send and process everything on the transmit
     * list.
     */
    void sendDma();

    /**
     * Handle a response packet by updating the corresponding DMA
     * request state to reflect the bytes received, and also update
     * the pending request counter. If the DMA request that this
     * packet is part of is complete, then signal the completion event
     * if present, potentially with a delay added to it.
     *
     * @param pkt Response packet to handler
     * @param delay Additional delay for scheduling the completion event
     */
    void handleResp(PacketPtr pkt, Tick delay = 0);

    struct DmaReqState : public Packet::SenderState
    {
        /** Event to call on the device when this transaction (all packets)
         * complete. */
        Event *completionEvent;

        /** Total number of bytes that this transaction involves. */
        const Addr totBytes;

        /** Number of bytes that have been acked for this transaction. */
        Addr numBytes;

        /** Amount to delay completion of dma by */
        const Tick delay;

        DmaReqState(Event *ce, Addr tb, Tick _delay)
            : completionEvent(ce), totBytes(tb), numBytes(0), delay(_delay)
        {}
    };

    /** The device that owns this port. */
    MemObject *device;

    /** Use a deque as we never do any insertion or removal in the middle */
    std::deque<PacketPtr> transmitList;

    /** Event used to schedule a future sending from the transmit list. */
    EventWrapper<PARDg5VDmaPort, &PARDg5VDmaPort::sendDma> sendEvent;

    /** The system that device/port are in. This is used to select which mode
     * we are currently operating in. */
    System *sys;

    /** Id for all requests */
    const MasterID masterId;

    /** Number of outstanding packets the dma port has. */
    uint32_t pendingCount;

    /** If we need to drain, keep the drain event around until we're done
     * here.*/
    DrainManager *drainManager;

    /** If the port is currently waiting for a retry before it can
     * send whatever it is that it's sending. */
    bool inRetry;

    long long int time_stamp_guest[GUESTNUM];
    long long int throughput_guest[GUESTNUM];
    
    void update_guest_bandwidth(GuestID guestID, int bytes);

  protected:

    bool recvTimingResp(PacketPtr pkt);
    void recvRetry() ;

    void queueDma(PacketPtr pkt);

  public:

    PARDg5VDmaPort(MemObject *dev, System *s);

    void dmaAction(Packet::Command cmd, Addr addr, int size, GuestID guestID, Event *event,
                   uint8_t *data, Tick delay, Request::Flags flag = 0);

    bool dmaPending() const { return pendingCount > 0; }

    unsigned int drain(DrainManager *drainManger);
};

class PARDg5VDmaDevice : public PioDevice
{
  protected:
    PARDg5VDmaPort dmaPort;
    
    void EncryptSector(uint8_t *data) {
    	for (int i = 0; i < SectorSize / 2; i++) {
    		uint8_t temp = data[i];
    		data[i] = data[SectorSize - 1 -i];
    		data[SectorSize - 1 -i] = temp;
    	}
    }
    
    void Encrypt(int size, uint8_t *data) {
        for (int i = 0; i < size; i += SectorSize) {
            EncryptSector(&data[i]);
        }
    }

    void DecryptSector(uint8_t *data) {
    	for (int i = 0; i < SectorSize / 2; i++) {
    		uint8_t temp = data[i];
    		data[i] = data[SectorSize - 1 -i];
    		data[SectorSize - 1 -i] = temp;
    	}
    }
    
    void Decrypt(int size, uint8_t *data) {
        for (int i = 0; i < size; i += SectorSize) {
            DecryptSector(&data[i]);
        }
    }

  public:
    typedef PARDg5VDmaDeviceParams Params;
    PARDg5VDmaDevice(const Params *p);
    virtual ~PARDg5VDmaDevice() { }

    void dmaWrite(Addr addr, int size, GuestID guestID, bool need_decrypt, Event *event, uint8_t *data,
                  Tick delay = 0)
    {
        if (need_decrypt) {
            Decrypt(size, data);
        }
        
        dmaPort.dmaAction(MemCmd::WriteReq, addr, size, guestID, event, data, delay);
    }

    void dmaRead(Addr addr, int size, GuestID guestID, bool need_encrypt, Event *event, uint8_t *data,
                 Tick delay = 0)
    {
        dmaPort.dmaAction(MemCmd::ReadReq, addr, size, guestID, event, data, delay);
        
        if (need_encrypt) {
            Encrypt(size, data);
        }
    }

    bool dmaPending() const { return dmaPort.dmaPending(); }

    virtual void init();

    unsigned int drain(DrainManager *drainManger);

    unsigned int cacheBlockSize() const { return sys->cacheLineSize(); }

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);

};

#endif // __HYPER_DEV_DMA_DEVICE_HH__
