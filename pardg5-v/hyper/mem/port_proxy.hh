/*
 * Copyright (c) 2014 ACS, ICT
 * All rights reserved
 *
 * Authors: Jiuyue Ma
 */

#ifndef __HYPER_MEM_PORT_PROXY_HH__
#define __HYPER_MEM_PORT_PROXY_HH__

#include "mem/port_proxy.hh"

class PARDg5VPortProxy : public PortProxy
{
  private:

    /** The actual physical port used by this proxy. */
    MasterPort &_port;

    /** Granularity of any transactions issued through this proxy. */
    const unsigned int _cacheLineSize;

    /** The actual physical port used by this proxy. */
    int _tag;

    void blobHelper(Addr addr, uint8_t *p, int size, MemCmd cmd) const;

  public:
    PARDg5VPortProxy(MasterPort &port, unsigned int cacheLineSize, int tag) :
        PortProxy(port, cacheLineSize), _port(port), _cacheLineSize(cacheLineSize), _tag(tag) { }
    virtual ~PARDg5VPortProxy() { }

    void updateTag(int tag) { _tag = tag; }

    /**
     * Read size bytes memory at address and store in p.
     */
    virtual void readBlob(Addr addr, uint8_t* p, int size) const
    { blobHelper(addr, p, size, MemCmd::ReadReq); }

    /**
     * Write size bytes from p to address.
     */
    virtual void writeBlob(Addr addr, uint8_t* p, int size) const
    { blobHelper(addr, p, size, MemCmd::WriteReq); }

    /**
     * Fill size bytes starting at addr with byte value val.
     */
    virtual void memsetBlob(Addr addr, uint8_t v, int size) const;

};

#endif // __HYPER_MEM_PORT_PROXY_HH__
