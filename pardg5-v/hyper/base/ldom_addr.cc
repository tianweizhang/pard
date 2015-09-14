#include <iostream>

#include "base/ldom_addr.hh"

std::ostream& operator<<(std::ostream &os, const LDomAddrTLBEntry &entry)
{
    os<<entry._domid<<","<<entry._start<<","<<entry._size<<","
      <<entry._remapped;
    return os;
}

