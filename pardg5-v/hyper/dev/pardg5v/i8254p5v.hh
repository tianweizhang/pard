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
 * Copyright (c) 2008 The Regents of The University of Michigan
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
 * Authors: Gabe Black
 *          Jiuyue Ma
 */

#ifndef __HYPER_DEV_PARDG5V_I8254P5V_HH__
#define __HYPER_DEV_PARDG5V_I8254P5V_HH__

#include "dev/intel_8254_timer.hh"
#include "dev/io_device.hh"
#include "params/I8254P5V.hh"

namespace PARDg5VISA
{

class IntSourcePin;

}

namespace X86ISA
{
class I8254P5V : public BasicPioDevice
{
  protected:
    Tick latency;
    class X86Intel8254Timer : public Intel8254Timer
    {
      protected:
        I8254P5V * parent;
        GuestID guestID;

        void
        counterInterrupt(unsigned int num)
        {
            parent->counterInterrupt(guestID, num);
        }

      public:
        X86Intel8254Timer(const std::string &name, const GuestID _guestID,
                          I8254P5V * _parent) :
            Intel8254Timer(_parent, name), parent(_parent), guestID(_guestID)
        {}
    };

    
    std::vector<X86Intel8254Timer *> pit;

    PARDg5VISA::IntSourcePin *intPin;

    int max_guests;
    
    void counterInterrupt(GuestID guestID, unsigned int num);

  public:
    typedef I8254P5VParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    I8254P5V(Params *p);
    virtual ~I8254P5V();

    Tick read(PacketPtr pkt);

    Tick write(PacketPtr pkt);

/*
    bool
    outputHigh(unsigned int num)
    {
        return pit.outputHigh(num);
    }

    uint8_t
    readCounter(unsigned int num)
    {
        return pit.readCounter(num);
    }
*/

    void
    writeCounter(GuestID guestID, unsigned int num, const uint8_t data)
    {
        pit[guestID]->writeCounter(num, data);
    }

    void
    writeControl(GuestID guestID, uint8_t val)
    {
        pit[guestID]->writeControl(val);
    }

    virtual void serialize(std::ostream &os);
    virtual void unserialize(Checkpoint *cp, const std::string &section);

};

} // namespace PARDg5VISA

#endif //__HYPER_DEV_PARDG5V_I8254P5V_HH__
