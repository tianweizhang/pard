# Copyright (c) 2014 ACS, ICT
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# Copyright (c) 2005-2007 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nathan Binkert
#          Jiuyue Ma

from m5.SimObject import SimObject
from m5.params import *
from PARDg5VDevice import PARDg5VPciDevice
from Ide import IdeID

class PARDg5VIdeDisk(SimObject):
    type = 'PARDg5VIdeDisk'
    cxx_header = "hyper/dev/ide_disk.hh"
    delay = Param.Latency('1us', "Fixed disk delay in microseconds")
    driveID = Param.IdeID('master', "Drive ID")
    image = Param.DiskImage("Disk image")
    guestID = Param.GuestID(0, "GuestID of this disk")

class PARDg5VIdeController(PARDg5VPciDevice):
    type = 'PARDg5VIdeController'
    cxx_header = "hyper/dev/ide_ctrl.hh"
    guestNum = Param.Int(1, "One guest for guestId 0")
    encrypt_enable_array = VectorParam.Int([0], "Encrypt enable array indexed by guestId. 0 represent disable, others represent enable.")
    disks = VectorParam.PARDg5VIdeDisk("IDE disks attached to this controller")

    VendorID = 0x8086
    DeviceID = 0x7111
    Command = 0x0
    Status = 0x280
    Revision = 0x0
    ClassCode = 0x01
    SubClassCode = 0x01
    ProgIF = 0x85
    BAR0 = 0x00000001
    BAR1 = 0x00000001
    BAR2 = 0x00000001
    BAR3 = 0x00000001
    BAR4 = 0x00000001
    BAR5 = 0x00000001
    InterruptLine = 0x1f
    InterruptPin = 0x01
    BAR0Size = '8B'
    BAR1Size = '4B'
    BAR2Size = '8B'
    BAR3Size = '4B'
    BAR4Size = '16B'

    io_shift = Param.UInt32(0x0, "IO port shift");
    ctrl_offset = Param.UInt32(0x0, "IDE disk control offset")

    num_channels = Param.Int(8, "Number of IDE channels")
