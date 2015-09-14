#ifndef __HYPER_BASE_LDOM_ADDR_HH__
#define __HYPER_BASE_LDOM_ADDR_HH__

#include "base/addr_range.hh"
#include "hyper/base/guest_addr_range.hh"
#include "hyper/base/pardg5v.hh"

/**
 * LDom Address Mapper
 **/
class LDomAddrTLBEntry
{
  private:
    friend std::ostream& operator<<(std::ostream &os, const LDomAddrTLBEntry &entry);

  protected:
    LDomID _domid;
    Addr _start;
    Addr _size;
    Addr _remapped;

  public:
    LDomAddrTLBEntry(LDomID domid, Addr start, Addr size, Addr remapped)
        : _domid(domid), _start(start), _size(size), _remapped(remapped) {}
    LDomAddrTLBEntry(GuestAddrRange original, AddrRange remapped)
        : _domid(original.guestID()),
          _start(original.start()), _size(original.size()),
          _remapped(remapped.start())
    {
        assert(!original.interleaved() && !remapped.interleaved());
    }

  public:
    bool contains(const LDomID &domid, const Addr &a) const {
        return (_domid == domid && a >= _start && a <= _start+_size-1);
    }
    Addr translate(const Addr &addr)  const {
        return (addr - _start + _remapped);
    }

  public:
    LDomID domid() const  { return _domid; }
    Addr start() const    { return _start; }
    Addr size() const     { return _size; }
    Addr remapped() const { return _remapped; }
};
//typedef std::vector<LDomAddrTLBEntry> LDomAddrTLB;

class LDomAddrTLB
{
  private:
    std::vector<LDomAddrTLBEntry> tlb_entries;

  public:
    void push_back(LDomID ldomID, Addr addr, Addr size, Addr remapped) {
        tlb_entries.push_back(LDomAddrTLBEntry(ldomID, addr, size, remapped));
    }
    void push_back(const LDomAddrTLBEntry &entry) {
        tlb_entries.push_back(entry);
    }

    const int size() const { return tlb_entries.size(); }

    void remove(LDomID ldomID)
    {
        std::vector<LDomAddrTLBEntry>::iterator it = tlb_entries.begin();
        while (it != tlb_entries.end()) {
            if (it->domid() == ldomID)
                it = tlb_entries.erase(it);
            else
                it++;
        }
    }

    void query(LDomID ldomID, std::vector<LDomAddrTLBEntry> &xent)
    {
        std::vector<LDomAddrTLBEntry>::const_iterator it = tlb_entries.begin();
        while (it != tlb_entries.end()) {
            if (it->domid() == ldomID)
                return xent.push_back(*it);
            it++;
        }
    }

    const Addr remapAddr(LDomID ldomID, Addr addr) const
    {
        std::vector<LDomAddrTLBEntry>::const_iterator it = tlb_entries.begin();
        while (it != tlb_entries.end()) {
            if (it->contains(ldomID, addr))
                return it->translate(addr);
            it++;
        }
        return addr;
    }
};

#endif	// __HYPER_BASE_LDOM_ADDR_HH__
