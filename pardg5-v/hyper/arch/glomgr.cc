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

#include <signal.h>
#include <string.h>
#include <sys/signal.h>
#include <unistd.h>

#include <cstdio>
#include <string>

#include "base/misc.hh"
#include "base/socket.hh"
#include "base/trace.hh"
#include "cpu/thread_context.hh"
#include "debug/GMAll.hh"
#include "hyper/arch/glomgr.hh"
#include "hyper/arch/guest/system.hh"
#include "hyper/arch/system.hh"
#include "hyper/arch/PARDg5VLDomain.hh"
#include "sim/console.hh"

using namespace std; 
using namespace Debug;

/**
 * GlobalManagerAgent
 **/

PARDg5VGlobalManager *
ManagedResourceAgent::mgr()
{
    assert (_system);
    return _system->mgr;
}

ManagedResourceAgent *
ManagedResourceAgentParams::create()
{
    return new ManagedResourceAgent(this);
}


/**
 * PARDg5VGlobalManagerListener
 **/

PARDg5VGlobalManagerListener::Event::Event(
    PARDg5VGlobalManagerListener *l, int fd, int e)
    : PollEvent(fd, e), listener(l)
{}

void
PARDg5VGlobalManagerListener::
Event::process(int revent)
{
    listener->accept();
}

PARDg5VGlobalManagerListener::PARDg5VGlobalManagerListener(
    PARDg5VGlobalManager *_mgr, int _port)
    : event(NULL), mgr(_mgr), port(_port)
{
    assert(!mgr->listener);
    mgr->listener = this;
}

PARDg5VGlobalManagerListener::~PARDg5VGlobalManagerListener()
{
    if (event)
        delete event;
}

std::string
PARDg5VGlobalManagerListener::name()
{
    return mgr->name() + ".listener";
}

void
PARDg5VGlobalManagerListener::listen()
{
    if (ListenSocket::allDisabled()) {
        warn_once("Sockets disabled, not accepting gdb connections");
        return;
    }

    while (!listener.listen(port, true)) {
        DPRINTF(GMMisc, "Can't bind port %d\n", port);
        port++;
    }

    event = new Event(this, listener.getfd(), POLLIN);
    pollQueue.schedule(event);

    ccprintf(cerr, "%d: %s: listening for remote control on port %d\n",
             curTick(), name(), port);
}

void
PARDg5VGlobalManagerListener::accept()
{
    if (!listener.islistening())
        panic("PARDg5VGlobalManagerListener::accept(): cannot accept if we're not listening!");

    int sfd = listener.accept(true);

    if (sfd != -1) {
        if (mgr->isattached())
            close(sfd);
        else
            mgr->attach(sfd);
    }
}


/**
 * PARDg5VGlobalManager
 **/

PARDg5VGlobalManager::Event::Event(PARDg5VGlobalManager *_mgr, int fd, int e)
    : PollEvent(fd, e), mgr(_mgr)
{}

void
PARDg5VGlobalManager::Event::process(int revent)
{
    if (revent & POLLIN)
        mgr->process();
    else if (revent & POLLNVAL)
        mgr->detach();
}


void
PARDg5VGlobalManager::pardg5v_cmd_create_ldom(int argc, const char **argv, void *ctx)
{
    if (argc != 1) {
        printf("Invalid arguments.\n");
        return;
    }
    PARDg5VGlobalManager *mgr = (PARDg5VGlobalManager *)ctx;
    LDomID id = mgr->createLDom();
    printf("LDom #%d created.\n", id);
}

void
PARDg5VGlobalManager::pardg5v_cmd_startup_ldom(int argc, const char **argv, void *ctx)
{
    PARDg5VGlobalManager *mgr = (PARDg5VGlobalManager *)ctx;
    LDomID id = -1;
    if (argc != 2) {
        printf("Invalid arguments.\n");
        return;
    }

    id = atoi(argv[1]);
    mgr->startupLDomVM(id);
    printf("Startup LDom #%d ...\n", id);
}


PARDg5VGlobalManager::PARDg5VGlobalManager(X86PARDg5VSystem *_system)
    : event(NULL), listener(NULL), fd(-1), attached(false),
      system(_system)
{
    Console::regCmd("create",
                    PARDg5VGlobalManager::pardg5v_cmd_create_ldom, (void *)this,
                    "Create PARDg5-V logical domain");
    Console::regCmd("startup", 
                    PARDg5VGlobalManager::pardg5v_cmd_startup_ldom, (void *)this,
                    "Startup PARDg5-V logical domain");
}

PARDg5VGlobalManager::~PARDg5VGlobalManager()
{
    if (event)
        delete event;
}

void
PARDg5VGlobalManager::registerManagedResource(ManagedResource *res)
{
    if (resources.find(res->resname()) != resources.end())
        panic("managed resource \"%s\" already exist.", res->resname());
    resources[res->resname()] = res;
    DPRINTF(GlobalManager, "Register managed resource: \"%s\".\n", res->resname());
}

std::string
PARDg5VGlobalManager::name()
{
    return system->name() + ".global_manager";
}

bool
PARDg5VGlobalManager::isattached()
{ return attached; }

void
PARDg5VGlobalManager::attach(int f)
{
    fd = f;

    event = new Event(this, fd, POLLIN);
    pollQueue.schedule(event);

    attached = true;
    DPRINTF(GlobalManager, "PARDg5-V global manager attached\n");
}

void
PARDg5VGlobalManager::detach()
{
    attached = false;
    close(fd);
    fd = -1;

    pollQueue.remove(event);
    DPRINTF(GlobalManager, "PARDg5-V global manager detached\n");
}

uint8_t
PARDg5VGlobalManager::getbyte()
{
    uint8_t b;
    if (::read(fd, &b, 1) != 1) {
        warn("could not read byte from global manager.");
        return -1;
    }
    return b;
}

void
PARDg5VGlobalManager::putbyte(uint8_t b)
{
    if (::write(fd, &b, 1) != 1)
        warn("could not write byte to global manager");
}

// Send a packet to global manager
void
PARDg5VGlobalManager::send(const char *bp)
{
    char c;

    DPRINTF(GMSend, "send:  %s\n", bp);

    //Start sending a packet
    putbyte(GMStart);
    //Send the contents, and also keep a check sum.
    for (const char *p=bp; (c = *p); p++) {
        putbyte(c);
    }
    //Send the ending character.
    putbyte(GMEnd);
}

// Receive a packet from global manager
int
PARDg5VGlobalManager::recv(char *bp, int maxlen)
{
    char *p;
    uint8_t c;
    int len;

    do {
        p = bp;
        len = 0;
        // Find the beginning of a packet
        do {
            c = getbyte();
            // session error, kill it
            if (c == 0xFF)
                return -1;
        } while(c != GMStart);

        // Read until you find the end of the data in the packet
        while ((c = getbyte()) != GMEnd && len < maxlen) {
            *p++ = c;
            len++;
        }
        *p = '\0';

        if (len < maxlen) {
            putbyte(GMGoodP);
            break;
        }
        else {
            putbyte(GMBadP);
            continue;
        }
    } while(1);

    DPRINTF(GMRecv, "recv: %s\n", bp);

    return len;
}

bool
PARDg5VGlobalManager::process()
{
    size_t datalen;
    char data[GMPacketBufLen + 1];
    //const char *p;
    char command; //, subcmd;
    //bool ret;

    if (!attached)
        return false;

    for (;;) {
        if ((datalen = recv(data, sizeof(data))) == -1) {
            detach();
            return false;
        }
        data[sizeof(data) - 1] = 0; // Sentinel
        command = data[0];
        //subcmd = 0;
        //p = data + 1;

        char *strToken, *next_token;
        std::vector<std::string> args;
        strToken = strtok_r(data, ":", &next_token);
        while (strToken != NULL) {
            args.push_back(strToken);
            strToken = strtok_r(NULL, ":", &next_token);
        };

        switch (command) {
          case GMSet:
          {
            DPRINTF(GMMisc, "received GMSet: %s\n", data);
            if (args.size() != 5) {
                send("INV");
                break;
            }
            setResource(args[1],		// resname
                        atoi(args[2].c_str()),	// GuestID
                        atoi(args[3].c_str()),	// Index
                        args[4]);	// Value

            send("OK");
            break;
          }

          case GMQuery:
          {
            DPRINTF(GMMisc, "received GMQuery: %s\n", data);
            if (args.size() != 4) {
                send("INV");
                break;
            }
            std::string result = queryResource(
                args[1],               // resname
                atoi(args[2].c_str()), // GuestID
                atoi(args[3].c_str())); // Index
            send(result.c_str());
            break;
          }

          case GMCreateLDom:
            DPRINTF(GMMisc, "received GMCreateLDom: %s\n", data);
            createLDom();
            break;

          case GMDestroyLDom:
            DPRINTF(GMMisc, "received GMDestroyLDom: %s\n", data);
            if (args.size() != 2) {
                send("");
                break;
            }
            destroyLDom(atoi(args[1].c_str()));
            break;

          case GMStartupLDomVM:
            DPRINTF(GMMisc, "received GMStartupLDomVM: %s\n", data);
            if (args.size() != 2) {
                send("");
                break;
            }
            startupLDomVM(atoi(args[1].c_str()));
            break;

          case GMShutdownLDomVM:
            DPRINTF(GMMisc, "received GMShutdownLDomVM: %s\n", data);
            if (args.size() != 2) {
                send("");
                break;
            }
            shutdownLDomVM(atoi(args[1].c_str()));
            break;

          default:
            DPRINTF(GMMisc, "Unknown command: %c(%#x)\n", command, command);
            send("");
            break;
        }

        break;
    }

    return true;
}

ManagedResource *
PARDg5VGlobalManager::getResource(const std::string &resname)
{
    if (resources.find(resname) != resources.end())
        return resources[resname];
    else
        return NULL;
}

void
PARDg5VGlobalManager::setResource(const std::string &resname,
                                  GuestID guestID, int index,
                                  const std::string &value)
{
    ManagedResource *res;

    DPRINTF(GMMisc, "setResource: <%s, %d, %d> => %s\n",
            resname, guestID, index, value);

    if ((res = getResource(resname)) == NULL) {
        warn("try to set unknown resource: \"%s\"\n", resname);
        return;
    }

    res->set(guestID, index, value);
}

std::string
PARDg5VGlobalManager::queryResource(const std::string &resname,
                                    GuestID guestID, int index)
{
    ManagedResource *res;

    DPRINTF(GMMisc, "queryResource: <%s, %d, %d>\n", resname, guestID, index);


    if (NULL == (res = getResource(resname))) {
        warn("Try to query unknow resource: \"%s\"\n", resname);
        return "UNKNOWN";
    }

    return res->query(guestID, index);
}

LDomID
PARDg5VGlobalManager::createLDom()
{
    PARDg5VLDomain *ldom = system->createPARDg5VLDom();
    return ldom->getLDomID();
}

void
PARDg5VGlobalManager::destroyLDom(LDomID id)
{
    warn("GMDestroyLDom not impl.\n");
}

void
PARDg5VGlobalManager::startupLDomVM(LDomID id)
{
    DPRINTF(GlobalManager, "Startup logical domain #%d.\n", id);

    PARDg5VLDomain *pLDom = system->getPARDg5VLDomain(id);
    if (!pLDom) {
        send("INVALID");
        return;
    }

    ThreadContext *threadContext = system->getThreadContext(id);
    pLDom->initBSPState(threadContext);
    Cycles delay = ((ClockedObject*)threadContext->getCpuPtr())->ticksToCycles(
        simQuantum);
    threadContext->activate(delay);
}

void
PARDg5VGlobalManager::shutdownLDomVM(LDomID id)
{
    DPRINTF(GlobalManager, "Shutdown logical domain #%d.\n", id);

    PARDg5VLDomain *pLDom = system->getPARDg5VLDomain(id);
    ThreadContext *threadContext = system->getThreadContext(id);
    threadContext->suspend();
    pLDom->initBSPState(threadContext);
}

