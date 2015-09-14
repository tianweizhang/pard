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

/** @file
 * Implements a 8250-compatible UART with PARDg5-V GuestID support.
 */

#include <string>
#include <vector>

#include "base/inifile.hh"
#include "base/str.hh"        // for to_number
#include "base/trace.hh"
#include "config/the_isa.hh"
#include "debug/Uart.hh"
#include "dev/platform.hh"
#include "dev/terminal.hh"
#include "hyper/dev/pardg5v/platform.hh"
#include "hyper/dev/uart8250p5v.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

using namespace std;
using namespace TheISA;

Uart8250P5V::IntrEvent::IntrEvent(Uart8250P5V *u, int bit)
    : uart(u)
{
    DPRINTF(Uart, "UART Interrupt Event Initilizing\n");
    intrBit = bit;
}

const char *
Uart8250P5V::IntrEvent::description() const
{
    return "uart interrupt delay";
}

void
Uart8250P5V::IntrEvent::process()
{
    if (intrBit & uart->IER) {
       DPRINTF(Uart, "UART InterEvent, interrupting\n");
       uart->postConsoleInt();
       uart->status |= intrBit;
       uart->lastTxInt = curTick();
    }
    else
       DPRINTF(Uart, "UART InterEvent, not interrupting\n");

}

/* The linux serial driver (8250.c about line 1182) loops reading from
 * the device until the device reports it has no more data to
 * read. After a maximum of 255 iterations the code prints "serial8250
 * too much work for irq X," and breaks out of the loop. Since the
 * simulated system is so much slower than the actual system, if a
 * user is typing on the keyboard it is very easy for them to provide
 * input at a fast enough rate to not allow the loop to exit and thus
 * the error to be printed. This magic number provides a delay between
 * the time the UART receives a character to send to the simulated
 * system and the time it actually notifies the system it has a
 * character to send to alleviate this problem. --Ali
 */
void
Uart8250P5V::IntrEvent::scheduleIntr()
{
    static const Tick interval = 225 * SimClock::Int::ns;
    DPRINTF(Uart, "Scheduling IER interrupt for %#x, at cycle %lld\n", intrBit,
            curTick() + interval);
    if (!scheduled())
        uart->schedule(this, curTick() + interval);
    else
        uart->reschedule(this, curTick() + interval);
}


Uart8250P5V::Uart8250P5V(const Params *p)
    : Uart(p, 8), IER(0), DLAB(0), LCR(0), MCR(0), lastTxInt(0),
      txIntrEvent(this, TX_INT), rxIntrEvent(this, RX_INT)
{
    assert (p->guestID < 0xFF);
    guestID = p->guestID;
}

Tick
Uart8250P5V::read(PacketPtr pkt)
{
    assert(pkt->getAddr() >= pioAddr && pkt->getAddr() < pioAddr + pioSize);
    assert(pkt->getSize() == 1);

    Addr daddr = pkt->getAddr() - pioAddr;
    pkt->allocate();

    DPRINTF(Uart, " read register %#x\n", daddr);

    switch (daddr) {
        case 0x0:
            if (!(LCR & 0x80)) { // read byte
                if (term->dataAvailable())
                    pkt->set(term->in());
                else {
                    pkt->set((uint8_t)0);
                    // A limited amount of these are ok.
                    DPRINTF(Uart, "empty read of RX register\n");
                }
                status &= ~RX_INT;
                platform->clearConsoleInt();

                if (term->dataAvailable() && (IER & UART_IER_RDI))
                    rxIntrEvent.scheduleIntr();
            } else { // dll divisor latch
               ;
            }
            break;
        case 0x1:
            if (!(LCR & 0x80)) { // Intr Enable Register(IER)
                pkt->set(IER);
            } else { // DLM divisor latch MSB
                ;
            }
            break;
        case 0x2: // Intr Identification Register (IIR)
            DPRINTF(Uart, "IIR Read, status = %#x\n", (uint32_t)status);

            if (status & RX_INT) /* Rx data interrupt has a higher priority */
                pkt->set(IIR_RXID);
            else if (status & TX_INT) {
                pkt->set(IIR_TXID);
                //Tx interrupts are cleared on IIR reads
                status &= ~TX_INT;
            } else
                pkt->set(IIR_NOPEND);

            break;
        case 0x3: // Line Control Register (LCR)
            pkt->set(LCR);
            break;
        case 0x4: // Modem Control Register (MCR)
            pkt->set(MCR);
            break;
        case 0x5: // Line Status Register (LSR)
            uint8_t lsr;
            lsr = 0;
            // check if there are any bytes to be read
            if (term->dataAvailable())
                lsr = UART_LSR_DR;
            lsr |= UART_LSR_TEMT | UART_LSR_THRE;
            pkt->set(lsr);
            break;
        case 0x6: // Modem Status Register (MSR)
            pkt->set((uint8_t)0);
            break;
        case 0x7: // Scratch Register (SCR)
            pkt->set((uint8_t)0); // doesn't exist with at 8250.
            break;
        default:
            panic("Tried to access a UART port that doesn't exist\n");
            break;
    }
/*    uint32_t d32 = *data;
    DPRINTF(Uart, "Register read to register %#x returned %#x\n", daddr, d32);
*/
    pkt->makeAtomicResponse();
    return pioDelay;
}

Tick
Uart8250P5V::write(PacketPtr pkt)
{

    assert(pkt->getAddr() >= pioAddr && pkt->getAddr() < pioAddr + pioSize);
    assert(pkt->getSize() == 1);

    Addr daddr = pkt->getAddr() - pioAddr;

    DPRINTF(Uart, " write register %#x value %#x\n", daddr, pkt->get<uint8_t>());

    switch (daddr) {
        case 0x0:
            if (!(LCR & 0x80)) { // write byte
                term->out(pkt->get<uint8_t>());
                platform->clearConsoleInt();
                status &= ~TX_INT;
                if (UART_IER_THRI & IER)
                    txIntrEvent.scheduleIntr();
            } else { // dll divisor latch
               ;
            }
            break;
        case 0x1:
            if (!(LCR & 0x80)) { // Intr Enable Register(IER)
                IER = pkt->get<uint8_t>();
                if (UART_IER_THRI & IER)
                {
                    DPRINTF(Uart, "IER: IER_THRI set, scheduling TX intrrupt\n");
                    if (curTick() - lastTxInt > 225 * SimClock::Int::ns) {
                        DPRINTF(Uart, "-- Interrupting Immediately... %d,%d\n",
                                curTick(), lastTxInt);
                        txIntrEvent.process();
                    } else {
                        DPRINTF(Uart, "-- Delaying interrupt... %d,%d\n",
                                curTick(), lastTxInt);
                        txIntrEvent.scheduleIntr();
                    }
                }
                else
                {
                    DPRINTF(Uart, "IER: IER_THRI cleared, descheduling TX intrrupt\n");
                    if (txIntrEvent.scheduled())
                        deschedule(txIntrEvent);
                    if (status & TX_INT)
                        platform->clearConsoleInt();
                    status &= ~TX_INT;
                }

                if ((UART_IER_RDI & IER) && term->dataAvailable()) {
                    DPRINTF(Uart, "IER: IER_RDI set, scheduling RX intrrupt\n");
                    rxIntrEvent.scheduleIntr();
                } else {
                    DPRINTF(Uart, "IER: IER_RDI cleared, descheduling RX intrrupt\n");
                    if (rxIntrEvent.scheduled())
                        deschedule(rxIntrEvent);
                    if (status & RX_INT)
                        platform->clearConsoleInt();
                    status &= ~RX_INT;
                }
             } else { // DLM divisor latch MSB
                ;
            }
            break;
        case 0x2: // FIFO Control Register (FCR)
            break;
        case 0x3: // Line Control Register (LCR)
            LCR = pkt->get<uint8_t>();
            break;
        case 0x4: // Modem Control Register (MCR)
            if (pkt->get<uint8_t>() == (UART_MCR_LOOP | 0x0A))
                    MCR = 0x9A;
            break;
        case 0x7: // Scratch Register (SCR)
            // We are emulating a 8250 so we don't have a scratch reg
            break;
        default:
            panic("Tried to access a UART port that doesn't exist\n");
            break;
    }
    pkt->makeAtomicResponse();
    return pioDelay;
}

void
Uart8250P5V::dataAvailable()
{
    // if the kernel wants an interrupt when we have data
    if (IER & UART_IER_RDI)
    {
        this->postConsoleInt();
        status |= RX_INT;
    }

}

void
Uart8250P5V::postConsoleInt()
{
    DPRINTF(Uart, "Post console int => Guest#%d\n", guestID);
    static_cast<PARDg5VPlatform *>(platform)->postConsoleInt(guestID);
}


AddrRangeList
Uart8250P5V::getAddrRanges() const
{
    AddrRangeList ranges;
    ranges.push_back(RangeSize(pioAddr, pioSize));
    return ranges;
}

void
Uart8250P5V::serialize(ostream &os)
{
    SERIALIZE_SCALAR(status);
    SERIALIZE_SCALAR(IER);
    SERIALIZE_SCALAR(DLAB);
    SERIALIZE_SCALAR(LCR);
    SERIALIZE_SCALAR(MCR);
    Tick rxintrwhen;
    if (rxIntrEvent.scheduled())
        rxintrwhen = rxIntrEvent.when();
    else
        rxintrwhen = 0;
    Tick txintrwhen;
    if (txIntrEvent.scheduled())
        txintrwhen = txIntrEvent.when();
    else
        txintrwhen = 0;
     SERIALIZE_SCALAR(rxintrwhen);
     SERIALIZE_SCALAR(txintrwhen);
}

void
Uart8250P5V::unserialize(Checkpoint *cp, const std::string &section)
{
    UNSERIALIZE_SCALAR(status);
    UNSERIALIZE_SCALAR(IER);
    UNSERIALIZE_SCALAR(DLAB);
    UNSERIALIZE_SCALAR(LCR);
    UNSERIALIZE_SCALAR(MCR);
    Tick rxintrwhen;
    Tick txintrwhen;
    UNSERIALIZE_SCALAR(rxintrwhen);
    UNSERIALIZE_SCALAR(txintrwhen);
    if (rxintrwhen != 0)
        schedule(rxIntrEvent, rxintrwhen);
    if (txintrwhen != 0)
        schedule(txIntrEvent, txintrwhen);
}

Uart8250P5V *
Uart8250P5VParams::create()
{
    return new Uart8250P5V(this);
}
