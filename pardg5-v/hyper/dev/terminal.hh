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
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
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
 *          Ali Saidi
 *          Jiuyue Ma
 */

/* @file
 * User Terminal Interface
 */

/* @update
 * Allow multiple COM port connect to same terminal
 */

#ifndef __HYPER_DEV_TERMINAL_HH__
#define __HYPER_DEV_TERMINAL_HH__

#include <iostream>
#include <string>
#include <map>

#include "base/circlebuf.hh"
#include "base/pollevent.hh"
#include "base/socket.hh"
#include "cpu/intr_control.hh"
#include "params/PARDg5VTerminal.hh"
#include "sim/sim_object.hh"

class TerminalListener;
class Uart;

namespace PARDg5VISA {

class Terminal : public SimObject
{
  protected:
    class ListenEvent : public PollEvent
    {
      protected:
        Terminal *term;

      public:
        ListenEvent(Terminal *t, int fd, int e);
        void process(int revent);
    };

    friend class ListenEvent;
    ListenEvent *listenEvent;

    class InternalData;
    class DataEvent : public PollEvent
    {
      protected:
        Terminal *term;
        InternalData *internal;

      public:
        DataEvent(Terminal *t, InternalData *i, int fd, int e);
        void process(int revent);
    };

    friend class DataEvent;

  protected:
    class InternalData {
      public:
        static bool output;
        static std::string outfile_basename;

      public:
        Uart *uart;
        DataEvent * dataEvent;

      public:
        int number;
        int data_fd;

      public:
        CircleBuf txbuf;
        CircleBuf rxbuf;
        std::ostream *outfile;
    #if TRACING_ON == 1
        CircleBuf linebuf;
    #endif

      public:
        InternalData(Uart *_uart, int _number);
        ~InternalData();
    };

    std::map<int, InternalData *> uartTerm;

  public:
    typedef PARDg5VTerminalParams Params;
    Terminal(const Params *p);
    ~Terminal();

    void registerUart(Uart *uart, int number);

  protected:
    ListenSocket listener;

    void listen(int port);
    void accept();

  public:
    ///////////////////////
    // Terminal Interface

    void data(InternalData *internal);
    void detach(InternalData *internal);
    size_t  read(InternalData *internal, uint8_t *buf, size_t len);
    size_t write(InternalData *internal, const uint8_t *buf, size_t len);
      void  read(InternalData *i, uint8_t &c) {  read(i, &c, 1); }
      void write(InternalData *i, uint8_t  c) { write(i, &c, 1); }

  public:
    /////////////////
    // OS interface

    // Get a character from the terminal.
    uint8_t  in(int number);

    // get a character from the terminal in the console specific format
    // corresponds to GETC:
    // retval<63:61>
    //     000: success: character received
    //     001: success: character received, more pending
    //     100: failure: no character ready
    //     110: failure: character received with error
    //     111: failure: character received with error, more pending
    // retval<31:0>
    //     character read from console
    //
    // Interrupts are cleared when the buffer is empty.
    uint64_t console_in(int number);

    // Send a character to the terminal
    void out(int number, char c);

    // Ask the terminal if data is available
    bool dataAvailable(int number) {
        assert(uartTerm.find(number) != uartTerm.end());
        return !uartTerm[number]->rxbuf.empty();
    }
};

}; // namespace PARDg5VISA

#endif // __HYPER_DEV_TERMINAL_HH__
