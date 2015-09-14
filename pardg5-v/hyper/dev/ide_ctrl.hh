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
 *          Miguel Serrano
 *          Jiuyue Ma
 */

/** @file
 * Simple PCI IDE controller with bus mastering capability and UDMA
 * modeled after controller in the Intel PIIX4 chip
 */

/** @update
 * majiuyue: Extend PCI IDE controller with PARDg5-V GuestID support.
 */

#ifndef __HYPER_IDE_CTRL_HH__
#define __HYPER_IDE_CTRL_HH__

#include "base/bitunion.hh"
#include "dev/io_device.hh"
#include "dev/pcireg.h"
#include "hyper/dev/pcidev.hh"
#include "params/PARDg5VIdeController.hh"

class PARDg5VIdeDisk;

/**
 * Device model for an Intel PIIX4 IDE controller
 */

class PARDg5VIdeController : public PARDg5VPciDevice
{
  private:
    // Bus master IDE status register bit fields
    BitUnion8(BMIStatusReg)
        Bitfield<6> dmaCap0;
        Bitfield<5> dmaCap1;
        Bitfield<2> intStatus;
        Bitfield<1> dmaError;
        Bitfield<0> active;
    EndBitUnion(BMIStatusReg)

    BitUnion8(BMICommandReg)
        Bitfield<3> rw;
        Bitfield<0> startStop;
    EndBitUnion(BMICommandReg)

    struct Channel
    {
        std::string _name;

        const std::string
        name()
        {
            return _name;
        }

        /** Command and control block registers */
        Addr cmdAddr, cmdSize, ctrlAddr, ctrlSize;

        /** Registers used for bus master interface */
        struct BMIRegs
        {
            BMICommandReg command;
            uint8_t reserved0;
            BMIStatusReg status;
            uint8_t reserved1;
            uint32_t bmidtp;
        } bmiRegs;

        /** IDE disks connected to this controller */
        PARDg5VIdeDisk *master, *slave;

        /** Currently selected disk */
        PARDg5VIdeDisk *selected;

        bool selectBit;

        void
        select(bool selSlave)
        {
            selectBit = selSlave;
            selected = selectBit ? slave : master;
        }

        void accessCommand(Addr offset, int size, uint8_t *data, bool read);
        void accessControl(Addr offset, int size, uint8_t *data, bool read);
        void accessBMI(Addr offset, int size, uint8_t *data, bool read);

        Channel(std::string newName, Addr _cmdSize, Addr _ctrlSize);
        ~Channel();

        void serialize(const std::string &base, std::ostream &os);
        void unserialize(const std::string &base, Checkpoint *cp,
            const std::string &section);
    };

    std::vector<Channel *> channels;
    int numChannels;

    /** Bus master interface (BMI) registers */
    Addr bmiAddr, bmiSize;

    /** Registers used in device specific PCI configuration */
    uint16_t primaryTiming, secondaryTiming;
    uint8_t deviceTiming;
    uint8_t udmaControl;
    uint16_t udmaTiming;
    uint16_t ideConfig;

    // Internal management variables
    bool ioEnabled;
    bool bmEnabled;

    uint32_t ioShift, ctrlOffset;
    
    uint32_t guestNum;
    bool* encrypt_enable_array;

    void dispatchAccess(PacketPtr pkt, bool read);

  public:
    typedef PARDg5VIdeControllerParams Params;
    const Params *params() const { return (const Params *)_params; }
    PARDg5VIdeController(Params *p);
    virtual ~PARDg5VIdeController();

    /** See if a disk is selected based on its pointer */
    bool isDiskSelected(PARDg5VIdeDisk *diskPtr);

    void intrPost(GuestID guestID);

    Tick writeConfig(PacketPtr pkt);
    Tick readConfig(PacketPtr pkt);

    void setDmaComplete(PARDg5VIdeDisk *disk);

    Tick read(PacketPtr pkt);
    Tick write(PacketPtr pkt);

    bool needCrypt(int guestId) {
        if ((guestId >= 0) && (guestId < guestNum)) {
            return encrypt_enable_array[guestId];
        } else {
            return false;
        }
    }

    void serialize(std::ostream &os);
    void unserialize(Checkpoint *cp, const std::string &section);
};
#endif // __IDE_CTRL_HH_
