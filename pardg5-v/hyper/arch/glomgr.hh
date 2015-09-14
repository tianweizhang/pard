/*
 * Copyright (c) 2014 ACS, ICT
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

#ifndef __HYPER_ARCH_GLOMGR_HH__
#define __HYPER_ARCH_GLOMGR_HH__

#include <map>
#include <string>

#include "base/pollevent.hh"
#include "base/socket.hh"
#include "hyper/base/pardg5v.hh"
#include "params/ManagedResourceAgent.hh"
#include "sim/sim_object.hh"

class X86PARDg5VSystem;
class PARDg5VGlobalManagerListener;
class PARDg5VGlobalManager;

class ManagedResourceAgent : public SimObject
{
  public:
    typedef ManagedResourceAgentParams Params;
    ManagedResourceAgent(Params *p) : SimObject(p), _system(NULL) {}
    ~ManagedResourceAgent() {}

  public:
    void system(X86PARDg5VSystem *system) { _system = system; }
    X86PARDg5VSystem * system() { return _system; }
    PARDg5VGlobalManager * mgr();

  protected:
    X86PARDg5VSystem *_system;
};

class ManagedResource
{
protected:
    std::string _resname;
    ManagedResourceAgent *_agent;

public:
    ManagedResource(std::string resname, ManagedResourceAgent *agent)
        : _resname(resname), _agent(agent) {}
    const std::string & resname() { return _resname; }

    virtual void set(uint64_t tag, int idx, const std::string &value) = 0;
    virtual const std::string query(uint64_t tag, int idx) = 0;

    virtual     void set(LDomID ldomID, uint32_t destAddr, uint64_t data) = 0;
    virtual uint64_t get(LDomID ldomID, uint32_t destAddr) = 0;
};

enum GMCommands
{
    // Resource Commands
    GMSet	= 's',	// set resource
    GMQuery	= 'q',	// query resource

    // Logical Domain Commands
    GMCreateLDom     = 'C',	// Create Logical Domain VM
    GMDestroyLDom    = 'D',	// Destroy Logical Domain VM
    GMStartupLDomVM  = 'S',	// Start Logical Domain VM
    GMShutdownLDomVM = 'T'	// Stop Logical Domain VM
};

const char GMStart = '$';
const char GMEnd = '#';
const char GMGoodP = '+';
const char GMBadP = '-';

const int GMPacketBufLen = 1024;

class PARDg5VGlobalManager
{
  private:
    friend class PARDg5VGlobalManagerListener;

  protected:
    std::map<const std::string, ManagedResource *> resources;

  protected:
    class Event : public PollEvent
    {
      protected:
        PARDg5VGlobalManager *mgr;

      public:
        Event(PARDg5VGlobalManager *_mgr, int fd, int e);
        void process(int revent);
    };
    friend class Event;
    Event *event;
    PARDg5VGlobalManagerListener *listener;

  protected:
    //The socket commands come in through
    int fd;
    bool attached;
    X86PARDg5VSystem *system;

  protected:
    uint8_t getbyte();
    void putbyte(uint8_t b);
    int recv(char *data, int len);
    void send(const char *data);

    virtual bool process();

  public:
    PARDg5VGlobalManager(X86PARDg5VSystem *system);
    virtual ~PARDg5VGlobalManager();

    void registerManagedResource(ManagedResource *res);

    void attach(int fd);
    void detach();
    bool isattached();
    std::string name();

  protected:
    ManagedResource * getResource(const std::string &resname);

  protected:
    // Resource Interface
    void setResource(const std::string &resname, GuestID guestID, int index,
                     const std::string &value);
    std::string queryResource(const std::string &resname, GuestID guestID, int index);

    // Logical Domain Interface
    LDomID createLDom();
    void destroyLDom(LDomID id);
    void startupLDomVM(LDomID id);
    void shutdownLDomVM(LDomID id);

  public:
    static void pardg5v_cmd_create_ldom(int argc, const char **argv, void *ctx);
    static void pardg5v_cmd_startup_ldom(int argc, const char **argv, void *ctx);
};

class PARDg5VGlobalManagerListener
{
  protected:
    class Event : public PollEvent
    {
      protected:
        PARDg5VGlobalManagerListener *listener;

      public:
        Event(PARDg5VGlobalManagerListener *l, int fd, int e);
        void process(int revent);
    };
    friend class Event;
    Event *event;

  protected:
    ListenSocket listener;
    PARDg5VGlobalManager *mgr;
    int port;

  public:
    PARDg5VGlobalManagerListener(PARDg5VGlobalManager *_mgr, int _port);
    ~PARDg5VGlobalManagerListener();

    void accept();
    void listen();
    std::string name();
};

#endif
