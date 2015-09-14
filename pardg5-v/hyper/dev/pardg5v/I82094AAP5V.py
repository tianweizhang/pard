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
# Copyright (c) 2008 The Regents of The University of Michigan
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
# Authors: Gabe Black
#          Jiuyue Ma

##@update
# majiuyue: add support for PARDg5-V interrupt re-routing
#

from m5.params import *
from m5.proxy import *
from Device import BasicPioDevice
from PARDg5VIntPin import PARDg5VIntSinkPin
from X86IntPin import X86IntSinkPin

class I82094AAP5V(BasicPioDevice):
    type = 'I82094AAP5V'
    cxx_class = 'X86ISA::I82094AAP5V'
    cxx_header = "hyper/dev/pardg5v/i82094aap5v.hh"
    apic_id = Param.Int(1, 'APIC id for this IO APIC')
    int_master = MasterPort("Port for sending interrupt messages")
    int_latency = Param.Latency('1ns', \
            "Latency for an interrupt to propagate through this device.")
    external_int_pic = Param.I8259(NULL, "External PIC, if any")

    max_guests = Param.Int(8, "Maximum guests support")
    pin_mask = VectorParam.UInt64([0xFFFFFF, 0, 0, 0, 0, 0, 0, 0], \
            "Interrupt pin mask for each guest")

    def pin(self, line):
        return PARDg5VIntSinkPin(device=self, number=line)

    def x86pin(self, line):
        return X86IntSinkPin(device=self, number=line)

