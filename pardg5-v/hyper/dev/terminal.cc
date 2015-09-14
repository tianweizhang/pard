/*
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
 */

/* @file
 * Implements the user interface to a serial terminal
 */

#include <sys/ioctl.h>
#include <sys/termios.h>
#include <poll.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "base/atomicio.hh"
#include "base/misc.hh"
#include "base/output.hh"
#include "base/socket.hh"
#include "base/trace.hh"
#include "debug/Terminal.hh"
#include "debug/TerminalVerbose.hh"
#include "dev/platform.hh"
#include "dev/uart.hh"
#include "hyper/dev/terminal.hh"

using namespace std;

namespace PARDg5VISA {

/*
 * Poll event for the listen socket
 */
Terminal::ListenEvent::ListenEvent(Terminal *t, int fd, int e)
    : PollEvent(fd, e), term(t)
{
}

void
Terminal::ListenEvent::process(int revent)
{
    term->accept();
}

/*
 * Poll event for the data socket
 */
Terminal::DataEvent::DataEvent(Terminal *t, InternalData *i, int fd, int e)
    : PollEvent(fd, e), term(t), internal(i)
{
}

void
Terminal::DataEvent::process(int revent)
{
    if (revent & POLLIN)
        term->data(internal);
    else if (revent & POLLNVAL)
        term->detach(internal);
}

bool Terminal::InternalData::output = false;
std::string Terminal::InternalData::outfile_basename = "";

Terminal::InternalData::InternalData(Uart *_uart, int _number)
    : uart(_uart), dataEvent(NULL), number(_number), data_fd(-1),
      txbuf(16384), rxbuf(16384), outfile(NULL)
#if TRACING_ON == 1
      , linebuf(16384)
#endif
{
    if (output) {
        stringstream stream;
        ccprintf(stream, outfile_basename, number);
        outfile = simout.find(stream.str());
        if (!outfile)
            outfile = simout.create(stream.str());
        outfile->setf(ios::unitbuf);
    }
}

Terminal::InternalData::~InternalData()
{
    if (data_fd != -1)
        ::close(data_fd);
    if (dataEvent)
        delete dataEvent;
}

/*
 * Terminal code
 */
Terminal::Terminal(const Params *p)
    : SimObject(p), listenEvent(NULL)
{
    InternalData::output = p->output;
    if (p->output)
        InternalData::outfile_basename = p->name + ".com_%d";

    if (p->port)
        listen(p->port);
}

Terminal::~Terminal()
{
    if (listenEvent)
        delete listenEvent;
}

void
Terminal::registerUart(Uart *uart, int number)
{
    if (uartTerm.find(number) != uartTerm.end())
        panic("COM_%d (%s) already registered to terminal %s.",
              number, uart->name(), name());
    uartTerm[number] = new InternalData(uart, number);
}

///////////////////////////////////////////////////////////////////////
// socket creation and terminal attach
//

void
Terminal::listen(int port)
{
    if (ListenSocket::allDisabled()) {
        warn_once("Sockets disabled, not accepting terminal connections");
        return;
    }

    while (!listener.listen(port, true)) {
        DPRINTF(Terminal,
                ": can't bind address terminal port %d inuse PID %d\n",
                port, getpid());
        port++;
    }

    int p1, p2;
    p2 = name().rfind('.') - 1;
    p1 = name().rfind('.', p2);
    ccprintf(cerr, "Listening for %s connection on port %d\n",
            name().substr(p1+1,p2-p1), port);

    listenEvent = new ListenEvent(this, listener.getfd(), POLLIN);
    pollQueue.schedule(listenEvent);
}

void
Terminal::accept()
{
    if (!listener.islistening())
        panic("%s: cannot accept a connection if not listening!", name());

    int fd = listener.accept(true);

    // ReadIn Number
    int number;
    size_t ret;
    do { ret = ::read(fd, &number, sizeof(int)); } while (ret == -1 && errno == EINTR);
    if (ret < 0) {
        char message[] = "Can't get COM id from terminal!\n";
        atomic_write(fd, message, sizeof(message));
        ::close(fd);
        return;
    }

    // Get Internal Data
    if (uartTerm.find(number) == uartTerm.end()) {
        stringstream stream;
        ccprintf(stream, "COM %d not exist!\n", number);
        atomic_write(fd, stream.str().c_str(), stream.str().size());
        ::close(fd);
        return;
    }

    InternalData *internal = uartTerm[number];

    if (internal->data_fd != -1) {
        char message[] = "terminal already attached!\n";
        atomic_write(fd, message, sizeof(message));
        ::close(fd);
        return;
    }
    internal->data_fd = fd;

    internal->dataEvent = new DataEvent(this, internal, fd, POLLIN);
    pollQueue.schedule(internal->dataEvent);

    stringstream stream;
    ccprintf(stream, "==== m5 slave terminal: Terminal %d ====", number);

    // we need an actual carriage return followed by a newline for the
    // terminal
    stream << "\r\n";

    write(internal, (const uint8_t *)stream.str().c_str(), stream.str().size());

    DPRINTFN("attach terminal %d\n", number);

    internal->txbuf.readall(internal->data_fd);
}

void
Terminal::detach(InternalData *internal)
{
    if (internal->data_fd != -1) {
        ::close(internal->data_fd);
        internal->data_fd = -1;
    }

    pollQueue.remove(internal->dataEvent);
    delete internal->dataEvent;
    internal->dataEvent = NULL;

    DPRINTFN("detach terminal %d\n", internal->number);
}

void
Terminal::data(InternalData *internal)
{
    uint8_t buf[1024];
    int len;

    len = read(internal, buf, sizeof(buf));
    if (len) {
        internal->rxbuf.write((char *)buf, len);
        // Inform the UART there is data available
        internal->uart->dataAvailable();
    }
}

size_t
Terminal::read(InternalData *internal, uint8_t *buf, size_t len)
{
    int data_fd = internal->data_fd;

    if (data_fd < 0)
        panic("Terminal not properly attached.\n");

    size_t ret;
    do {
      ret = ::read(data_fd, buf, len);
    } while (ret == -1 && errno == EINTR);


    if (ret < 0)
        DPRINTFN("Read failed.\n");

    if (ret <= 0) {
        detach(internal);
        return 0;
    }

    return ret;
}

// Terminal output.
size_t
Terminal::write(InternalData *internal, const uint8_t *buf, size_t len)
{
    int data_fd = internal->data_fd;

    if (data_fd < 0)
        panic("Terminal not properly attached.\n");

    ssize_t ret = atomic_write(data_fd, buf, len);
    if (ret < len)
        detach(internal);

    return ret;
}

#define MORE_PENDING (ULL(1) << 61)
#define RECEIVE_SUCCESS (ULL(0) << 62)
#define RECEIVE_NONE (ULL(2) << 62)
#define RECEIVE_ERROR (ULL(3) << 62)

uint8_t
Terminal::in(int number)
{
    uint8_t c;
    InternalData *internal;

    assert(uartTerm.find(number) != uartTerm.end());
    internal = uartTerm[number];

    assert(!internal->rxbuf.empty());
    internal->rxbuf.read((char *)&c, 1);

    DPRINTF(TerminalVerbose, "[%d]in: \'%c\' %#02x more: %d\n",
            number, isprint(c) ? c : ' ', c, !internal->rxbuf.empty());

    return c;
}

uint64_t
Terminal::console_in(int number)
{
    uint64_t value;
    InternalData *internal;

    assert(uartTerm.find(number) != uartTerm.end());
    internal = uartTerm[number];

    if (dataAvailable(number)) {
        value = RECEIVE_SUCCESS | in(number);
        if (!internal->rxbuf.empty())
            value  |= MORE_PENDING;
    } else {
        value = RECEIVE_NONE;
    }

    DPRINTF(TerminalVerbose, "[%d]console_in: return: %#x\n", number, value);

    return value;
}

void
Terminal::out(int number, char c)
{
    InternalData *internal;

    assert(uartTerm.find(number) != uartTerm.end());
    internal = uartTerm[number];

#if TRACING_ON == 1
    if (DTRACE(Terminal)) {
        static char last = '\0';

        if ((c != '\n' && c != '\r') || (last != '\n' && last != '\r')) {
            if (c == '\n' || c == '\r') {
                int size = internal->linebuf.size();
                char *buffer = new char[size + 1];
                internal->linebuf.read(buffer, size);
                buffer[size] = '\0';
                DPRINTF(Terminal, "%s\n", buffer);
                delete [] buffer;
            } else {
                internal->linebuf.write(c);
            }
        }

        last = c;
    }
#endif

    internal->txbuf.write(c);

    if (internal->data_fd >= 0)
        write(internal, c);

    if (internal->outfile)
        internal->outfile->write(&c, 1);

    DPRINTF(TerminalVerbose, "[%d]out: \'%c\' %#02x\n",
            number, isprint(c) ? c : ' ', (int)c);

}

}; // namespace PARDg5VISA

PARDg5VISA::Terminal *
PARDg5VTerminalParams::create()
{
    return new PARDg5VISA::Terminal(this);
}

