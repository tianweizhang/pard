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

#ifndef __HYPER_GM_CPAGENT_HH__
#define __HYPER_GM_CPAGENT_HH__

#include "mem/mem_object.hh"
#include "mem/tport.hh"
#include "mem/mport.hh"
#include "mem/packet.hh"
#include "hyper/arch/glomgr.hh"
#include "params/CPConnector.hh"

class CPConnector : public MemObject
{
  protected:

    class CPConnectorSlavePort : public SimpleTimingPort
    {
      protected:
        CPConnector *agent;

      public:
        CPConnectorSlavePort(const std::string& _name, CPConnector *_agent)
          : SimpleTimingPort(_name, static_cast<MemObject *>(_agent)), agent(_agent)
        {}

        AddrRangeList getAddrRanges() const { return agent->getAddrRanges(); }

        Tick recvAtomic(PacketPtr pkt)
        {
            // @todo someone should pay for this
            pkt->busFirstWordDelay = pkt->busLastWordDelay = 0;
            return agent->recvAtomic(pkt);
        }
    };

    class CPConnectorMasterPort : public MessageMasterPort
    {
      protected:
        CPConnector *agent;
      
      public:
        CPConnectorMasterPort(const std::string& _name, CPConnector *_agent)
          : MessageMasterPort(_name, static_cast<MemObject *>(_agent)), agent(_agent)
        {}

        Tick recvResponse(PacketPtr pkt) { return agent->recvResponse(pkt); }

        void sendMessage() {}
    };

    /** Slave/Master ports of the bridge. */
    CPConnectorSlavePort slavePort;
    CPConnectorMasterPort masterPort;

  public:

    typedef CPConnectorParams Params;
    CPConnector(Params *p);

    virtual void init();

    virtual BaseMasterPort& getMasterPort(const std::string& if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort& getSlavePort(const std::string& if_name,
                                        PortID idx = InvalidPortID);

  public:

    Tick recvAtomic(PacketPtr pkt);
    Tick recvResponse(PacketPtr pkt);
    AddrRangeList getAddrRanges() const;

  protected:

    ManagedResource *resource;

  public:

    void registerManagedResource(ManagedResource *res);

  protected:

    int cpDevID;
    struct CPConnRegs {
        uint32_t cpType;
        uint8_t  cpIdent[12];
        uint8_t  cpCmd;
        uint8_t  cpState;
        uint16_t cpLDomID;
        uint32_t cpDestAddr;
        uint64_t cpData;
    } regs;

/*
    char cpIdent[12];
    int  cpType;
*/

    const Params *_param;
    const Params * param() const { return _param; } 

};

#endif //__HYPER_GM_CPAGENT_HH__
