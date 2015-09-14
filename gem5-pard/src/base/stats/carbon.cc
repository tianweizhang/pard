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

#if defined(__APPLE__)
#define _GLIBCPP_USE_C99 1
#endif

#if defined(__sun)
#include <math.h>
#endif

#include <cassert>
#ifdef __SUNPRO_CC
#include <math.h>
#endif
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/stats/carbon.hh"
#include "base/stats/info.hh"
#include "base/cast.hh"
#include "base/misc.hh"
#include "base/str.hh"

using namespace std;

#ifndef NAN
float __nan();
/** Define Not a number. */
#define NAN (__nan())
/** Need to define __nan() */
#define __M5_NAN
#endif

#ifdef __M5_NAN
float
__nan()
{
    union {
        uint32_t ui;
        float f;
    } nan;

    nan.ui = 0x7fc00000;
    return nan.f;
}
#endif

static int
remote_connect(const char *host, const char *port, struct addrinfo hints)
{
    struct addrinfo *res, *res0;
    int s, error;

    if ((error = getaddrinfo(host, port, &hints, &res)))
        panic("getaddrinfo: %s", gai_strerror(error));

    res0 = res;
    do {
        if ((s = socket(res0->ai_family, res0->ai_socktype,
                        res0->ai_protocol)) < 0)
            continue;

        if (connect(s, res0->ai_addr, res0->ai_addrlen) == 0)
            break;

        close(s);
        s = -1;
    } while ((res0 = res0->ai_next) != NULL);

    freeaddrinfo(res);

    return (s);
}

namespace Stats {

std::list<Info *> &statsList();

Carbon::Carbon()
    : sockfd(-1), host("127.0.0.1"), port("2003"),
      catalog("gem5test"), descriptions(false)
{
    ts = time(NULL);
}

Carbon::~Carbon()
{
    if (sockfd != -1)
        close(sockfd);
}

bool
Carbon::valid() const
{
    return true;
}

void
Carbon::begin()
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sockfd = remote_connect(host.c_str(), port.c_str(), hints);

    if (sockfd == -1)
        warn("connect to carbon host \"%s:%s\" failed.", host.c_str(), port.c_str());

    //ts++;
    ts = time(NULL);
}

void
Carbon::end()
{
    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }
}

bool
Carbon::noOutput(const Info &info)
{
    if (0 == regexec(&filter, info.name.c_str(), 0, NULL, 0))
        return false;

    const char * matchs[10] = {
        "system.mem_ctrls.addr_mapper.bw_total",
        "system.l2.tags.guestCapacity",
        "system.cpu0.ipc", "system.switch_cpus0.ipc",
        "system.cpu1.ipc", "system.switch_cpus1.ipc",
        "system.cpu2.ipc", "system.switch_cpus2.ipc",
        "system.cpu3.ipc", "system.switch_cpus3.ipc"
    };

    for (int i=0; i<10; i++) {
        if (strncmp(info.name.c_str(), matchs[i], strlen(matchs[i])) == 0)
            return false;
    }

    return true;
}

void
Carbon::generate(const std::string &key, Result value)
{
    if (sockfd == -1)
        return;
    std::stringstream result;
    result<<catalog<<"."<<key<<" "<<ValueToString(value, 2)<<" "<<ts<<endl;
    if(write(sockfd, result.str().c_str(), result.str().size())) {};
}

void
Carbon::visit(const ScalarInfo &info)
{
    if (noOutput(info))
        return;
    generate(info.name, info.result());
}

void
Carbon::visit(const VectorInfo &info)
{
    if (noOutput(info))
        return;

    for (off_type i = 0; i < info.size(); ++i) {
        if (!info.subnames[i].empty())
            generate(info.name + "." + info.subnames[i], info.result()[i]);
        else
            generate(info.name, info.result()[i]);
    }
}

void Carbon::visit(const FormulaInfo &info)
{
    visit((const VectorInfo &)info);
}


#define CARBON_UNIMPL(fun)					\
    if (noOutput(info))						\
        return;							\
    panic("Un-Implement Carbon statistics type %s\n", #fun);

void Carbon::visit(const Vector2dInfo &info)   { CARBON_UNIMPL(Vector2d); }
void Carbon::visit(const DistInfo &info)       { CARBON_UNIMPL(Dist); }
void Carbon::visit(const VectorDistInfo &info) { CARBON_UNIMPL(VectorDist); }
void Carbon::visit(const SparseHistInfo &info) { CARBON_UNIMPL(SparseHist); }

Output *
initCarbon(const std::string &host, const std::string &catalog,
           const std::string &regex, bool desc)
{
    static Carbon carbon;
    static bool connected = false;

    if (!connected) {
        carbon.setCarbonHost(host, "2003");
        carbon.setCatalog(catalog);
        carbon.setFilter(regex);
        carbon.descriptions = desc;
        connected = true;
    }

    return &carbon;
}

} // namespace Stats
