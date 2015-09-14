/*
 * Copyright (c) 2013 ARM Limited
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
 * Authors: Andrew Schultz
 *          Ali Saidi
 *          Miguel Serrano
 *          Jiuyue Ma
 */

/** @update
 * majiuyue: Extend PCI IDE controller with PARDg5-V GuestID support.
 */

#include <string>

#include "cpu/intr_control.hh"
#include "debug/IdeCtrl.hh"
#include "hyper/base/pardg5v.hh"
// #include "debug/IdeBW.hh"
#include "hyper/dev/ide_ctrl.hh"
#include "hyper/dev/ide_disk.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/PARDg5VIdeController.hh"
#include "sim/byteswap.hh"

// clang complains about std::set being overloaded with Packet::set if
// we open up the entire namespace std
using std::string;

// Bus master IDE registers
enum BMIRegOffset {
    BMICommand = 0x0,
    BMIStatus = 0x2,
    BMIDescTablePtr = 0x4
};

// PCI config space registers
enum ConfRegOffset {
    PrimaryTiming = 0x40,
    SecondaryTiming = 0x42,
    DeviceTiming = 0x44,
    UDMAControl = 0x48,
    UDMATiming = 0x4A,
    IDEConfig = 0x54
};

static const uint16_t timeRegWithDecodeEn = 0x8000;

PARDg5VIdeController::Channel::Channel(
        string newName, Addr _cmdSize, Addr _ctrlSize) :
    _name(newName),
    cmdAddr(0), cmdSize(_cmdSize), ctrlAddr(0), ctrlSize(_ctrlSize),
    master(NULL), slave(NULL), selected(NULL)
{
    memset(&bmiRegs, 0, sizeof(bmiRegs));
    bmiRegs.status.dmaCap0 = 1;
    bmiRegs.status.dmaCap1 = 1;
}

PARDg5VIdeController::Channel::~Channel()
{
}

PARDg5VIdeController::PARDg5VIdeController(Params *p)
    : PARDg5VPciDevice(p), numChannels(p->num_channels),
    bmiAddr(0), bmiSize(BARSize[4]),
    primaryTiming(htole(timeRegWithDecodeEn)),
    secondaryTiming(htole(timeRegWithDecodeEn)),
    deviceTiming(0), udmaControl(0), udmaTiming(0), ideConfig(0),
    ioEnabled(false), bmEnabled(false),
    ioShift(p->io_shift), ctrlOffset(p->ctrl_offset)
{
    if (params()->disks.size() != 2*numChannels)
        panic("IDE controllers(%d channels) must have %d devices attached!\n",
               numChannels, 2*numChannels);

    guestNum = params()->guestNum;
    int encrypt_enable_array_size = params()->encrypt_enable_array.size();
    if (guestNum != encrypt_enable_array_size) {
        panic("GuestNum should equal to the size of encrypt_enable_array in IDE controllers!\n");
    } else {
        encrypt_enable_array = new bool[guestNum];
        // printf("the guestNum in IDE controller is %d.\n", guestNum);
        // DPRINTF(IdeBW, "the guestNum in IDE controller is %d.\n", guestNum);
        for (int i = 0; i < guestNum; i++) {
            encrypt_enable_array[i] = (params()->encrypt_enable_array[i] != 0);
            // printf("encrypt_enable_array[%d] is %s.\n", i, encrypt_enable_array[i]==true?"TRUE":"FALSE");
            // DPRINTF(IdeBW, "encrypt_enable_array[%d] is %s.\n", i, encrypt_enable_array[i]==true?"TRUE":"FALSE");
        }
    }

    // Attach disk to controller
    for (int i = 0; i < params()->disks.size(); i++) {
        params()->disks[i]->setController(this);
    }

    // Create all channels
    channels.resize(numChannels);
    for (int i=0; i<numChannels; i++) {
        // create channel
        channels[i] = new Channel(name() + csprintf(".channel%d", i),
                                  BARSize[0], BARSize[1]);
        // assign the disks to channels
        channels[i]->master = params()->disks[i*2];
        channels[i]->slave  = params()->disks[i*2 + 1];
        channels[i]->select(false);

        if ((BARAddrs[0] & ~BAR_IO_MASK) && (!legacyIO[0] || ioShift)) {
            channels[i]->cmdAddr  = BARAddrs[0];
            channels[i]->cmdSize  = BARSize[0];
            channels[i]->ctrlAddr = BARAddrs[1];
            channels[i]->ctrlSize = BARSize[1];
        }
    }

    ioEnabled = (config.command & htole(PCI_CMD_IOSE));
    bmEnabled = (config.command & htole(PCI_CMD_BME));
}

PARDg5VIdeController::~PARDg5VIdeController()
{
    for (int i=0; i<numChannels; i++)
        delete channels[i];
}

bool
PARDg5VIdeController::isDiskSelected(PARDg5VIdeDisk *diskPtr)
{
    for (int i=0; i<numChannels; i++)
        if (channels[i]->selected == diskPtr)
            return true;
    return false;
}

void
PARDg5VIdeController::intrPost(GuestID guestID)
{
    Channel *channel = channels[guestID];
    channel->bmiRegs.status.intStatus = 1;
    PARDg5VPciDevice::intrPost(guestID);
}

void
PARDg5VIdeController::setDmaComplete(PARDg5VIdeDisk *disk)
{
    Channel *channel = NULL;

    for (int i=0; i<numChannels; i++) {
        if (disk == channels[i]->master || disk == channels[i]->slave) {
            channel = channels[i];
        }
    }

    if (!channel)
        panic("Unable to find disk based on pointer %#x\n", disk);

    channel->bmiRegs.command.startStop = 0;
    channel->bmiRegs.status.active = 0;
    channel->bmiRegs.status.intStatus = 1;
}

Tick
PARDg5VIdeController::readConfig(PacketPtr pkt)
{
    GuestID guest = PARDG5V_PKT_TO_GUESTID(pkt);
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
    if (offset < PCI_DEVICE_SPECIFIC) {
        return PARDg5VPciDevice::readConfig(pkt);
    }

    pkt->allocate();

    switch (pkt->getSize()) {
      case sizeof(uint8_t):
        switch (offset) {
          case DeviceTiming:
            pkt->set<uint8_t>(deviceTiming);
            break;
          case UDMAControl:
            pkt->set<uint8_t>(udmaControl);
            break;
          case PrimaryTiming + 1:
            pkt->set<uint8_t>(bits(htole(primaryTiming), 15, 8));
            break;
          case SecondaryTiming + 1:
            pkt->set<uint8_t>(bits(htole(secondaryTiming), 15, 8));
            break;
          case IDEConfig:
            pkt->set<uint8_t>(bits(htole(ideConfig), 7, 0));
            break;
          case IDEConfig + 1:
            pkt->set<uint8_t>(bits(htole(ideConfig), 15, 8));
            break;
          default:
            panic("Invalid PCI configuration read for size 1 at offset: %#x!\n",
                    offset);
        }
        DPRINTF(IdeCtrl, "Guest#%d: PCI read offset: %#x size: 1 data: %#x\n", guest, offset,
                (uint32_t)pkt->get<uint8_t>());
        break;
      case sizeof(uint16_t):
        switch (offset) {
          case PrimaryTiming:
            pkt->set<uint16_t>(primaryTiming);
            break;
          case SecondaryTiming:
            pkt->set<uint16_t>(secondaryTiming);
            break;
          case UDMATiming:
            pkt->set<uint16_t>(udmaTiming);
            break;
          case IDEConfig:
            pkt->set<uint16_t>(ideConfig);
            break;
          default:
            panic("Guest#%d: Invalid PCI configuration read for size 2 offset: %#x!\n",
                    guest, offset);
        }
        DPRINTF(IdeCtrl, "Guest#%d: PCI read offset: %#x size: 2 data: %#x\n", guest, offset,
                (uint32_t)pkt->get<uint16_t>());
        break;
      case sizeof(uint32_t):
        if (offset == IDEConfig)
            pkt->set<uint32_t>(ideConfig);
        else
            panic("No 32bit reads implemented for this device.");
        DPRINTF(IdeCtrl, "Guest#%d: PCI read offset: %#x size: 4 data: %#x\n", guest, offset,
                (uint32_t)pkt->get<uint32_t>());
        break;
      default:
        panic("Guest#%d: invalid access size(?) for PCI configspace!\n", guest);
    }
    pkt->makeAtomicResponse();
    return configDelay;
}


Tick
PARDg5VIdeController::writeConfig(PacketPtr pkt)
{
    GuestID guest = PARDG5V_PKT_TO_GUESTID(pkt);
    Channel *channel = channels[guest];

    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
    if (offset < PCI_DEVICE_SPECIFIC) {
        PARDg5VPciDevice::writeConfig(pkt);
    } else {
        switch (pkt->getSize()) {
          case sizeof(uint8_t):
            switch (offset) {
              case DeviceTiming:
                deviceTiming = pkt->get<uint8_t>();
                break;
              case UDMAControl:
                udmaControl = pkt->get<uint8_t>();
                break;
              case IDEConfig:
                replaceBits(ideConfig, 7, 0, pkt->get<uint8_t>());
                break;
              case IDEConfig + 1:
                replaceBits(ideConfig, 15, 8, pkt->get<uint8_t>());
                break;
              default:
                panic("Guest#%d: Invalid PCI configuration write "
                        "for size 1 offset: %#x!\n", guest, offset);
            }
            DPRINTF(IdeCtrl, "Guest#%d: PCI write offset: %#x size: 1 data: %#x\n",
                    guest, offset, (uint32_t)pkt->get<uint8_t>());
            break;
          case sizeof(uint16_t):
            switch (offset) {
              case PrimaryTiming:
                primaryTiming = pkt->get<uint16_t>();
                break;
              case SecondaryTiming:
                secondaryTiming = pkt->get<uint16_t>();
                break;
              case UDMATiming:
                udmaTiming = pkt->get<uint16_t>();
                break;
              case IDEConfig:
                ideConfig = pkt->get<uint16_t>();
                break;
              default:
                panic("Guest#%d: Invalid PCI configuration write "
                        "for size 2 offset: %#x!\n",
                        guest, offset);
            }
            DPRINTF(IdeCtrl, "Guest#%d: PCI write offset: %#x size: 2 data: %#x\n",
                    guest, offset, (uint32_t)pkt->get<uint16_t>());
            break;
          case sizeof(uint32_t):
            if (offset == IDEConfig)
                ideConfig = pkt->get<uint32_t>();
            else
                panic("Write of unimplemented PCI config. register: %x\n", offset);
            break;
          default:
            panic("invalid access size(?) for PCI configspace!\n");
        }
        pkt->makeAtomicResponse();
    }

    /* Trap command register writes and enable IO/BM as appropriate as well as
     * BARs. */
    switch(offset) {
      case PCI0_BASE_ADDR0:
        if (BARAddrs[0] != 0)
            channel->cmdAddr = BARAddrs[0];
        break;

      case PCI0_BASE_ADDR1:
        if (BARAddrs[1] != 0)
            channel->ctrlAddr = BARAddrs[1];
        break;

/*
      case PCI0_BASE_ADDR2:
        if (BARAddrs[2] != 0)
            secondary.cmdAddr = BARAddrs[2];
        break;

      case PCI0_BASE_ADDR3:
        if (BARAddrs[3] != 0)
            secondary.ctrlAddr = BARAddrs[3];
        break;
*/

      case PCI0_BASE_ADDR4:
        if (BARAddrs[4] != 0)
            bmiAddr = BARAddrs[4];
        break;

      case PCI_COMMAND:
        DPRINTF(IdeCtrl, "Guest#%d: Writing to PCI Command val: %#x\n", guest, config.command);
        ioEnabled = (config.command & htole(PCI_CMD_IOSE));
        bmEnabled = (config.command & htole(PCI_CMD_BME));
        break;
    }
    return configDelay;
}

void
PARDg5VIdeController::Channel::accessCommand(Addr offset,
        int size, uint8_t *data, bool read)
{
    const Addr SelectOffset = 6;
    const uint8_t SelectDevBit = 0x10;

    if (!read && offset == SelectOffset)
        select(*data & SelectDevBit);

    if (selected == NULL) {
        assert(size == sizeof(uint8_t));
        *data = 0;
    } else if (read) {
        selected->readCommand(offset, size, data);
    } else {
        selected->writeCommand(offset, size, data);
    }
}

void
PARDg5VIdeController::Channel::accessControl(Addr offset,
        int size, uint8_t *data, bool read)
{
    if (selected == NULL) {
        assert(size == sizeof(uint8_t));
        *data = 0;
    } else if (read) {
        selected->readControl(offset, size, data);
    } else {
        selected->writeControl(offset, size, data);
    }
}

void
PARDg5VIdeController::Channel::accessBMI(Addr offset,
        int size, uint8_t *data, bool read)
{
    assert(offset + size <= sizeof(BMIRegs));
    if (read) {
        memcpy(data, (uint8_t *)&bmiRegs + offset, size);
    } else {
        switch (offset) {
          case BMICommand:
            {
                if (size != sizeof(uint8_t))
                    panic("Invalid BMIC write size: %x\n", size);

                BMICommandReg oldVal = bmiRegs.command;
                BMICommandReg newVal = *data;

                // if a DMA transfer is in progress, R/W control cannot change
                if (oldVal.startStop && oldVal.rw != newVal.rw)
                    oldVal.rw = newVal.rw;

                if (oldVal.startStop != newVal.startStop) {
                    if (selected == NULL)
                        panic("DMA start for disk which does not exist\n");

                    if (oldVal.startStop) {
                        DPRINTF(IdeCtrl, "Stopping DMA transfer\n");
                        bmiRegs.status.active = 0;

                        selected->abortDma();
                    } else {
                        DPRINTF(IdeCtrl, "Starting DMA transfer\n");
                        bmiRegs.status.active = 1;

                        selected->startDma(letoh(bmiRegs.bmidtp));
                    }
                }

                bmiRegs.command = newVal;
            }
            break;
          case BMIStatus:
            {
                if (size != sizeof(uint8_t))
                    panic("Invalid BMIS write size: %x\n", size);

                BMIStatusReg oldVal = bmiRegs.status;
                BMIStatusReg newVal = *data;

                // the BMIDEA bit is read only
                newVal.active = oldVal.active;

                // to reset (set 0) IDEINTS and IDEDMAE, write 1 to each
                if ((oldVal.intStatus == 1) && (newVal.intStatus == 1)) {
                    newVal.intStatus = 0; // clear the interrupt?
                } else {
                    // Assigning two bitunion fields to each other does not
                    // work as intended, so we need to use this temporary variable
                    // to get around the bug.
                    uint8_t tmp = oldVal.intStatus;
                    newVal.intStatus = tmp;
                }
                if ((oldVal.dmaError == 1) && (newVal.dmaError == 1)) {
                    newVal.dmaError = 0;
                } else {
                    uint8_t tmp = oldVal.dmaError;
                    newVal.dmaError = tmp;
                }

                bmiRegs.status = newVal;
            }
            break;
          case BMIDescTablePtr:
            if (size != sizeof(uint32_t))
                panic("Invalid BMIDTP write size: %x\n", size);
            bmiRegs.bmidtp = htole(*(uint32_t *)data & ~0x3);
            break;
          default:
            if (size != sizeof(uint8_t) && size != sizeof(uint16_t) &&
                    size != sizeof(uint32_t))
                panic("IDE controller write of invalid write size: %x\n", size);
            memcpy((uint8_t *)&bmiRegs + offset, data, size);
        }
    }
}

void
PARDg5VIdeController::dispatchAccess(PacketPtr pkt, bool read)
{
    pkt->allocate();
    if (pkt->getSize() != 1 && pkt->getSize() != 2 && pkt->getSize() !=4)
         panic("Bad IDE read size: %d\n", pkt->getSize());

    if (!ioEnabled) {
        pkt->makeAtomicResponse();
        DPRINTF(IdeCtrl, "io not enabled\n");
        return;
    }

    GuestID guest = PARDG5V_PKT_TO_GUESTID(pkt);
    Channel *channel = channels[guest];
    Addr addr = pkt->getAddr();
    int size = pkt->getSize();
    uint8_t *dataPtr = pkt->getPtr<uint8_t>();

    if (addr >= channel->cmdAddr &&
            addr < (channel->cmdAddr + channel->cmdSize)) {
        addr -= channel->cmdAddr;
        // linux may have shifted the address by ioShift,
        // here we shift it back, similarly for ctrlOffset.
        addr >>= ioShift;
        channel->accessCommand(addr, size, dataPtr, read);
    } else if (addr >= channel->ctrlAddr &&
               addr < (channel->ctrlAddr + channel->ctrlSize)) {
        addr -= channel->ctrlAddr;
        addr += ctrlOffset;
        channel->accessControl(addr, size, dataPtr, read);
    } else if (addr >= bmiAddr && addr < (bmiAddr + bmiSize)) {
        if (!read && !bmEnabled)
            return;
        addr -= bmiAddr;
        assert(addr < sizeof(Channel::BMIRegs));
        channel->accessBMI(addr, size, dataPtr, read);
    } else {
        panic("IDE controller access to invalid address: %#x\n", addr);
    }

#ifndef NDEBUG
    uint32_t data;
    if (pkt->getSize() == 1)
        data = pkt->get<uint8_t>();
    else if (pkt->getSize() == 2)
        data = pkt->get<uint16_t>();
    else
        data = pkt->get<uint32_t>();
    DPRINTF(IdeCtrl, "Guest#%d: %s from offset: %#x size: %#x data: %#x\n",
            guest, read ? "Read" : "Write", pkt->getAddr(), pkt->getSize(), data);
#endif

    pkt->makeAtomicResponse();
}

Tick
PARDg5VIdeController::read(PacketPtr pkt)
{
    dispatchAccess(pkt, true);
    return pioDelay;
}

Tick
PARDg5VIdeController::write(PacketPtr pkt)
{
    dispatchAccess(pkt, false);
    return pioDelay;
}

void
PARDg5VIdeController::serialize(std::ostream &os)
{
    // Serialize the PARDg5VPciDevice base class
    PARDg5VPciDevice::serialize(os);

    // Serialize channels
    for (int i=0; i<numChannels; i++)
        channels[i]->serialize(csprintf("channel%d", i), os);

    // Serialize config registers
    SERIALIZE_SCALAR(primaryTiming);
    SERIALIZE_SCALAR(secondaryTiming);
    SERIALIZE_SCALAR(deviceTiming);
    SERIALIZE_SCALAR(udmaControl);
    SERIALIZE_SCALAR(udmaTiming);
    SERIALIZE_SCALAR(ideConfig);

    // Serialize internal state
    SERIALIZE_SCALAR(ioEnabled);
    SERIALIZE_SCALAR(bmEnabled);
    SERIALIZE_SCALAR(bmiAddr);
    SERIALIZE_SCALAR(bmiSize);
}

void
PARDg5VIdeController::Channel::serialize(const std::string &base, std::ostream &os)
{
    paramOut(os, base + ".cmdAddr", cmdAddr);
    paramOut(os, base + ".cmdSize", cmdSize);
    paramOut(os, base + ".ctrlAddr", ctrlAddr);
    paramOut(os, base + ".ctrlSize", ctrlSize);
    uint8_t command = bmiRegs.command;
    paramOut(os, base + ".bmiRegs.command", command);
    paramOut(os, base + ".bmiRegs.reserved0", bmiRegs.reserved0);
    uint8_t status = bmiRegs.status;
    paramOut(os, base + ".bmiRegs.status", status);
    paramOut(os, base + ".bmiRegs.reserved1", bmiRegs.reserved1);
    paramOut(os, base + ".bmiRegs.bmidtp", bmiRegs.bmidtp);
    paramOut(os, base + ".selectBit", selectBit);
}

void
PARDg5VIdeController::unserialize(Checkpoint *cp, const std::string &section)
{
    // Unserialize the PARDg5VPciDevice base class
    PARDg5VPciDevice::unserialize(cp, section);

    // Unserialize channels
    for (int i=0; i<numChannels; i++)
        channels[i]->unserialize(csprintf("channel%d", i), cp, section);

    // Unserialize config registers
    UNSERIALIZE_SCALAR(primaryTiming);
    UNSERIALIZE_SCALAR(secondaryTiming);
    UNSERIALIZE_SCALAR(deviceTiming);
    UNSERIALIZE_SCALAR(udmaControl);
    UNSERIALIZE_SCALAR(udmaTiming);
    UNSERIALIZE_SCALAR(ideConfig);

    // Unserialize internal state
    UNSERIALIZE_SCALAR(ioEnabled);
    UNSERIALIZE_SCALAR(bmEnabled);
    UNSERIALIZE_SCALAR(bmiAddr);
    UNSERIALIZE_SCALAR(bmiSize);
}

void
PARDg5VIdeController::Channel::unserialize(const std::string &base, Checkpoint *cp,
    const std::string &section)
{
    paramIn(cp, section, base + ".cmdAddr", cmdAddr);
    paramIn(cp, section, base + ".cmdSize", cmdSize);
    paramIn(cp, section, base + ".ctrlAddr", ctrlAddr);
    paramIn(cp, section, base + ".ctrlSize", ctrlSize);
    uint8_t command;
    paramIn(cp, section, base +".bmiRegs.command", command);
    bmiRegs.command = command;
    paramIn(cp, section, base + ".bmiRegs.reserved0", bmiRegs.reserved0);
    uint8_t status;
    paramIn(cp, section, base + ".bmiRegs.status", status);
    bmiRegs.status = status;
    paramIn(cp, section, base + ".bmiRegs.reserved1", bmiRegs.reserved1);
    paramIn(cp, section, base + ".bmiRegs.bmidtp", bmiRegs.bmidtp);
    paramIn(cp, section, base + ".selectBit", selectBit);
    select(selectBit);
}

PARDg5VIdeController *
PARDg5VIdeControllerParams::create()
{
    return new PARDg5VIdeController(this);
}
