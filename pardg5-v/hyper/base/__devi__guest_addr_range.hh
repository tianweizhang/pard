/*
 * Copyright (c) 2014 ACS, ICT
 * All rights reserved
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

#ifndef __HYPER_BASE_GUEST_ADDR_RANGE_HH__
#define __HYPER_BASE_GUEST_ADDR_RANGE_HH__

#include "base/addr_range.hh"
#include "hyper/pardg5v.hh"

class GuestAddrRange : public AddrRange
{

  private:

    /// Private fields for GuestID
    GuestID _guestID;

  public:

    GuestAddrRange()
        : AddrRange()
    {}

    GuestAddrRange(Addr _start, Addr _end, uint8_t _intlv_high_bit,
              uint8_t _intlv_bits, uint8_t _intlv_match, GuestID _guestID)
        : AddrRange(_start, _end, _intlv_high_bit, _intlv_bits, _intlv_match),
          _guestID(_guestID)
    {}

    GuestAddrRange(Addr _start, Addr _end, GuestID _guestID)
        : AddrRange(_start, _end),
          _guestID(_guestID)
    {}

    /**
     * Create an address range by merging a collection of interleaved
     * ranges.
     *
     * @param ranges Interleaved ranges to be merged
     */
/*
    GuestAddrRange(const std::vector<GuestAddrRange>& ranges)
        : AddrRange(ranges)
    {
        if (!ranges.empty()) {
            // get guestID from the first one
            _guestID = ranges.front()._guestID;
        }
    }
*/

    /**
     * Get the guest id of the range.
     */
    Addr guestID() const { return _guestID; }

    /**
     * Get a string representation of the range. This could
     * alternatively be implemented as a operator<<, but at the moment
     * that seems like overkill.
     */
    std::string to_string() const
    {
        return csprintf("[tag: %d], %s", _guestID, AddrRange::to_string().c_str());
    }

    /**
     * Determine if another range merges with the current one, i.e. if
     * they are part of the same contigous range and have the same
     * interleaving bits.
     *
     * @param r Range to evaluate merging with
     * @return true if the two ranges would merge
     */
    bool mergesWith(const GuestAddrRange& r) const
    {
        if (_guestID != r._guestID)
            return  false;
        return AddrRange::mergesWith(r);
    }

    /**
     * Determine if another range intersects this one, i.e. if there
     * is an address that is both in this range and the other
     * range. No check is made to ensure either range is valid.
     *
     * @param r Range to intersect with
     * @return true if the intersection of the two ranges is not empty
     */
    bool intersects(const GuestAddrRange& r) const
    {
        if (_guestID != r._guestID)
            return false;
        return AddrRange::intersects(r);
    }

    /**
     * Determine if this range is a subset of another range, i.e. if
     * every address in this range is also in the other range. No
     * check is made to ensure either range is valid.
     *
     * @param r Range to compare with
     * @return true if the this range is a subset of the other one
     */
    bool isSubset(const GuestAddrRange& r) const
    {
        if (_guestID != r._guestID)
            return false;
        return AddrRange::isSubset(r);
    }

    /**
     * Determine if the range contains an address.
     *
     * @param a Address to compare with
     * @return true if the address is in the range
     */
    bool contains(const Addr& a, const GuestID& guestID) const
    {
        if (_guestID != guestID)
             return false;

        return AddrRange::contains(a);
    }
};

inline GuestAddrRange
RangeEx(Addr start, Addr end, GuestID guestID)
{ return GuestAddrRange(start, end - 1, guestID); }

inline GuestAddrRange
RangeIn(Addr start, Addr end, GuestID guestID)
{ return GuestAddrRange(start, end, guestID); }

inline GuestAddrRange
RangeSize(Addr start, Addr size, GuestID guestID)
{ return GuestAddrRange(start, start + size - 1, guestID); }

#endif // __HYPER_BASE_GUEST_ADDR_RANGE_HH__
