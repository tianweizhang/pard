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

#ifndef __HYPER_DEV_PARDG5V_INTDEV_HH__
#define __HYPER_DEV_PARDG5V_INTDEV_HH__

#include <cassert>
#include <list>
#include <string>

#include "dev/x86/intdev.hh"
#include "hyper/base/pardg5v.hh"
#include "params/PARDg5VIntLine.hh"
#include "params/PARDg5VIntSinkPin.hh"
#include "params/PARDg5VIntSourcePin.hh"
#include "sim/sim_object.hh"

namespace PARDg5VISA {

class IntDevice : public X86ISA::IntDevice
{
  public:
    IntDevice(MemObject * parent, Tick latency = 0) :
        X86ISA::IntDevice(parent, latency)
    {
    }

    // for g++ warning: "hides overloaded virtual function [-Woverloaded-virtual]"
    using X86ISA::IntDevice::signalInterrupt;
    using X86ISA::IntDevice::raiseInterruptPin;
    using X86ISA::IntDevice::lowerInterruptPin;

    // our PARDg5-V addon interface, with GuestID
    virtual void
    signalInterrupt(GuestID guestID, int line)
    {
        panic("signalInterrupt(PARDg5-V extended) not implemented.\n");
    }

    virtual void
    raiseInterruptPin(GuestID guestID, int number)
    {
        panic("raiseInterruptPin(PARDg5-V extended) not implemented.\n");
    }

    virtual void
    lowerInterruptPin(GuestID guestID, int number)
    {
        panic("lowerInterruptPin(PARDg5-V extended) not implemented.\n");
    }
};

class IntSinkPin : public SimObject
{
  public:
    IntDevice * device;
    int number;

    typedef PARDg5VIntSinkPinParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    IntSinkPin(Params *p) : SimObject(p),
            device(dynamic_cast<IntDevice *>(p->device)), number(p->number)
    {
        assert(device);
    }
};

class IntSourcePin : public SimObject
{
  protected:
    std::vector<IntSinkPin *> sinks;

  public:
    typedef PARDg5VIntSourcePinParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    void
    addSink(IntSinkPin *sink)
    {
        sinks.push_back(sink);
    }

    void
    raise(GuestID guestID)
    {
        for (int i = 0; i < sinks.size(); i++) {
            const IntSinkPin &pin = *sinks[i];
            pin.device->raiseInterruptPin(guestID, pin.number);
        }
    }
    
    void
    lower(GuestID guestID)
    {
        for (int i = 0; i < sinks.size(); i++) {
            const IntSinkPin &pin = *sinks[i];
            pin.device->lowerInterruptPin(guestID, pin.number);
        }
    }

    IntSourcePin(Params *p) : SimObject(p)
    {}
};

class IntLine : public SimObject
{
  protected:
    IntSourcePin *source;
    IntSinkPin *sink;

  public:
    typedef PARDg5VIntLineParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    IntLine(Params *p) : SimObject(p), source(p->source), sink(p->sink)
    {
        source->addSink(sink);
    }
};

} // namespace X86ISA

#endif //__DEV_X86_INTDEV_HH__
