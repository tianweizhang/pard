/*
 * Copyright (c) 2014 ACS, ICT.
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

#ifndef __BASE_STATS_CARBON_HH__
#define __BASE_STATS_CARBON_HH__

#include <iosfwd>
#include <string>

#include <sys/types.h>
#include <regex.h>

#include "base/misc.hh"
#include "base/output.hh"
#include "base/stats/output.hh"
#include "base/stats/types.hh"

namespace Stats {

class Carbon : public Output
{
  protected:
    int sockfd;
    std::string host;
    std::string port;
    std::string catalog;
    regex_t filter;
    time_t ts;

  protected:
    bool noOutput(const Info &info);

  public:
    bool descriptions;

  public:
    Carbon();
    ~Carbon();

    void setCarbonHost(const std::string &_host,
                       const std::string &_port = "2003")
    {
        host = _host;
        port = _port;
    }

    void setCatalog(const std::string &_catalog) { catalog = _catalog; }
    void setFilter(const std::string &_regex) {
        int ret = regcomp(&filter, _regex.c_str(), REG_ICASE);
        panic_if(ret!=0, "carbon: bad filter regex [%s]\n", _regex.c_str());
    }


    // Implement Visit
    virtual void visit(const ScalarInfo &info);
    virtual void visit(const VectorInfo &info);
    virtual void visit(const DistInfo &info);
    virtual void visit(const VectorDistInfo &info);
    virtual void visit(const Vector2dInfo &info);
    virtual void visit(const FormulaInfo &info);
    virtual void visit(const SparseHistInfo &info);

    // Implement Output
    virtual bool valid() const;
    virtual void begin();
    virtual void end();

  protected:
    void generate(const std::string &key, Result value);
};

std::string ValueToString(Result value, int precision);

Output *initCarbon(const std::string &host, const std::string &catalog,
                   const std::string &regex, bool desc);

} // namespace Stats

#endif // __BASE_STATS_CARBON_HH__
