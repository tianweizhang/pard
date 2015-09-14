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
# Authors: Jiuyue Ma

from m5.params import *

class GuestID(CheckedInt):
    cxx_type = 'GuestID'
    size = 16
    unsigned = True

class GuestAddrRange(ParamValue):
    cxx_type = 'GuestAddrRange'

    def __init__(self, *args, **kwargs):
        # Disable interleaving by default
        self.intlvHighBit = 0
        self.intlvBits = 0
        self.intlvMatch = 0

        def handle_kwargs(self, kwargs):
            # An address range needs to have an upper limit, specified
            # either explicitly with an end, or as an offset using the
            # size keyword.
            if 'end' in kwargs:
                self.end = Addr(kwargs.pop('end'))
            elif 'size' in kwargs:
                self.end = self.start + Addr(kwargs.pop('size')) - 1
            else:
                raise TypeError, "Either end or size must be specified"

            # Now on to the optional bit
            if 'intlvHighBit' in kwargs:
                self.intlvHighBit = int(kwargs.pop('intlvHighBit'))
            if 'intlvBits' in kwargs:
                self.intlvBits = int(kwargs.pop('intlvBits'))
            if 'intlvMatch' in kwargs:
                self.intlvMatch = int(kwargs.pop('intlvMatch'))

        if 'guestID' in kwargs:
            self.guestID = int(kwargs.pop('guestID'))
        else:
            raise TypeError, "guestID must be specified in kwargs"

        if len(args) == 0:
            self.start = Addr(kwargs.pop('start'))
            handle_kwargs(self, kwargs)

        elif len(args) == 1:
            if kwargs:
                self.start = Addr(args[0])
                handle_kwargs(self, kwargs)
            elif isinstance(args[0], (list, tuple)):
                self.start = Addr(args[0][0])
                self.end = Addr(args[0][1])
            else:
                self.start = Addr(0)
                self.end = Addr(args[0]) - 1

        elif len(args) == 2:
            self.start = Addr(args[0])
            self.end = Addr(args[1])
        else:
            raise TypeError, "Too many arguments specified"

        if kwargs:
            raise TypeError, "Too many keywords: %s" % kwargs.keys()

    def __str__(self):
        return '(%s)%s:%s' % (self.guestID, self.start, self.end)

    def size(self):
        # Divide the size by the size of the interleaving slice
        return (long(self.end) - long(self.start) + 1) >> self.intlvBits

    @classmethod
    def cxx_predecls(cls, code):
        Addr.cxx_predecls(code)
        code('#include "hyper/base/guest_addr_range.hh"')

    @classmethod
    def swig_predecls(cls, code):
        Addr.swig_predecls(code)

    def getValue(self):
        # Go from the Python class to the wrapped C++ class generated
        # by swig
        from m5.internal.xrange import GuestAddrRange

        return GuestAddrRange(long(self.start), long(self.end),
                         int(self.intlvHighBit), int(self.intlvBits),
                         int(self.intlvMatch), int(self.guestID))

__all__ = ['GuestID', 'GuestAddrRangeParam']
