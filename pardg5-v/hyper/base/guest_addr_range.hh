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
 * Copyright (c) 2012 ARM Limited
 * All rights reserved
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
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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
 * Authors: Nathan Binkert
 *          Steve Reinhardt
 *          Andreas Hansson
 *          Jiuyue Ma
 */

#ifndef __HYPER_BASE_GUEST_ADDR_RANGE_HH__
#define __HYPER_BASE_GUEST_ADDR_RANGE_HH__

#include <vector>

#include "base/bitfield.hh"
#include "base/cprintf.hh"
#include "base/misc.hh"
#include "base/types.hh"
#include "hyper/base/pardg5v.hh"

class GuestAddrRange
{

  private:

    /// Private field for guest id
    GuestID _guestID;

    /// Private fields for the start and end of the range
    Addr _start;
    Addr _end;

    /// The high bit of the slice that is used for interleaving
    uint8_t intlvHighBit;

    /// The number of bits used for interleaving, set to 0 to disable
    uint8_t intlvBits;

    /// The value to compare the slice addr[high:(high - bits + 1)]
    /// with.
    uint8_t intlvMatch;

  public:

    GuestAddrRange()
        : _guestID(0), _start(1), _end(0), intlvHighBit(0), intlvBits(0), intlvMatch(0)
    {}

    GuestAddrRange(Addr _start, Addr _end, uint8_t _intlv_high_bit,
              uint8_t _intlv_bits, uint8_t _intlv_match, GuestID _guestID)
        : _guestID(_guestID), _start(_start), _end(_end), intlvHighBit(_intlv_high_bit),
          intlvBits(_intlv_bits), intlvMatch(_intlv_match)
    {}

    GuestAddrRange(Addr _start, Addr _end, GuestID _guestID)
        : _guestID(_guestID), _start(_start), _end(_end), intlvHighBit(0), intlvBits(0),
          intlvMatch(0)
    {}

    /**
     * Create an address range by merging a collection of interleaved
     * ranges.
     *
     * @param ranges Interleaved ranges to be merged
     */
    GuestAddrRange(const std::vector<GuestAddrRange>& ranges)
        : _guestID(0), _start(1), _end(0), intlvHighBit(0), intlvBits(0), intlvMatch(0)
    {
        if (!ranges.empty()) {
            // get the values from the first one and check the others
            _guestID = ranges.front()._guestID;
            _start = ranges.front()._start;
            _end = ranges.front()._end;
            intlvHighBit = ranges.front().intlvHighBit;
            intlvBits = ranges.front().intlvBits;

            if (ranges.size() != (ULL(1) << intlvBits))
                fatal("Got %d ranges spanning %d interleaving bits\n",
                      ranges.size(), intlvBits);

            uint8_t match = 0;
            for (std::vector<GuestAddrRange>::const_iterator r = ranges.begin();
                 r != ranges.end(); ++r) {
                if (!mergesWith(*r))
                    fatal("Can only merge ranges with the same guestID, start, end "
                          "and interleaving bits\n");

                if (r->intlvMatch != match)
                    fatal("Expected interleave match %d but got %d when "
                          "merging\n", match, r->intlvMatch);
                ++match;
            }

            // our range is complete and we can turn this into a
            // non-interleaved range
            intlvHighBit = 0;
            intlvBits = 0;
        }
    }

    /**
     * Determine if the range is interleaved or not.
     *
     * @return true if interleaved
     */
    bool interleaved() const { return intlvBits != 0; }

    /**
     * Determing the interleaving granularity of the range.
     *
     * @return The size of the regions created by the interleaving bits
     */
    uint64_t granularity() const
    {
        return ULL(1) << (intlvHighBit - intlvBits + 1);
    }

    /**
     * Determine the number of interleaved address stripes this range
     * is part of.
     *
     * @return The number of stripes spanned by the interleaving bits
     */
    uint32_t stripes() const { return ULL(1) << intlvBits; }

    /**
     * Get the guest id of the address range.
     */
    GuestID guestID() const { return _guestID; }

    /**
     * Get the size of the address range. For a case where
     * interleaving is used we make the simplifying assumption that
     * the size is a divisible by the size of the interleaving slice.
     */
    Addr size() const
    {
        return (_end - _start + 1) >> intlvBits;
    }

    /**
     * Determine if the range is valid.
     */
    bool valid() const { return _start < _end; }

    /**
     * Get the start address of the range.
     */
    Addr start() const { return _start; }

    /**
     * Get a string representation of the range. This could
     * alternatively be implemented as a operator<<, but at the moment
     * that seems like overkill.
     */
    std::string to_string() const
    {
        if (interleaved())
            return csprintf("(%d): [%#llx : %#llx], [%d : %d] = %d",
                            _guestID, _start, _end,
                            intlvHighBit, intlvHighBit - intlvBits + 1,
                            intlvMatch);
        else
            return csprintf("(%d): [%#llx : %#llx]", _guestID, _start, _end);
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
        return r._guestID == _guestID &&
            r._start == _start && r._end == _end &&
            r.intlvHighBit == intlvHighBit &&
            r.intlvBits == intlvBits;
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
        if (r._guestID != _guestID)
            return false;

        if (!interleaved()) {
            return _start <= r._end && _end >= r._start;
        }

        // the current range is interleaved, split the check up in
        // three cases
        if (r.size() == 1)
            // keep it simple and check if the address is within
            // this range
            return contains(r.start(), r._guestID);
        else if (!r.interleaved())
            // be conservative and ignore the interleaving
            return _start <= r._end && _end >= r._start;
        else if (mergesWith(r))
            // restrict the check to ranges that belong to the
            // same chunk
            return intlvMatch == r.intlvMatch;
        else
            panic("Cannot test intersection of interleaved range %s\n",
                  to_string());
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
        if (r._guestID != _guestID)
            return false;
        if (interleaved())
            panic("Cannot test subset of interleaved range %s\n", to_string());
        return _start >= r._start && _end <= r._end;
    }

    /**
     * Determine if the range contains an address.
     *
     * @param a Address to compare with
     * @param guestID GuestID of address
     * @return true if the address is in the range
     */
    bool contains(const Addr& a, const GuestID &guestID) const
    {
        if (_guestID != guestID)
            return false;

        // check if the address is in the range and if there is either
        // no interleaving, or with interleaving also if the selected
        // bits from the address match the interleaving value
        return a >= _start && a <= _end &&
            (!interleaved() ||
             (bits(a, intlvHighBit, intlvHighBit - intlvBits + 1) ==
              intlvMatch));
    }

/**
 * Keep the operators away from SWIG.
 */
#ifndef SWIG

    /**
     * Less-than operator used to turn an STL map into a binary search
     * tree of non-overlapping address ranges.
     *
     * @param r Range to compare with
     * @return true if the start address is less than that of the other range
     */
    bool operator<(const GuestAddrRange& r) const
    {
        if (_start != r._start)
            return _start < r._start;
        else
            // for now assume that the end is also the same, and that
            // we are looking at the same interleaving bits
            return intlvMatch < r.intlvMatch;
    }

#endif // SWIG
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
