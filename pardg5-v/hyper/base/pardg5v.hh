#ifndef __HYPER_BASE_PARDG5V_HH__
#define __HYPER_BASE_PARDG5V_HH__

#include <stdint.h>

typedef uint16_t GuestID;

const GuestID ALL_GUEST = 0xFF;

#define QOSTAG_TO_GUESTID(tag) ((GuestID)((tag) & 0xFF))
#define SET_GUESTID(tag, guest) (((uint64_t)(tag)&0xFFFFFFFFFFFFFF00)|((uint64_t)(guest)&0xFF))

#define PARDG5V_PKT_TO_GUESTID(pkt) ((GuestID)(pkt->getQosTag() & 0xFF))
#define PARDG5V_SET_PKT_GUESTID(pkt, guestID) {                \
    uint64_t qosTag = (pkt->getQosTag()) & 0xFFFFFFFFFFFFFF00; \
    qosTag |= (guestID & 0xFF);                                \
    pkt->setQosTag(qosTag);                                    \
}

// Logical Domain
typedef uint16_t LDomID;
const LDomID INVALID_LDOM = 0xFFFF;
#define QOSTAG_TO_LDOMID(tag) ((LDomID)((tag) & 0xFFFF))

#endif	// __HYPER_BASE_PARDG5V_HH__
