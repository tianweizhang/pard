#ifndef __SIM_CONSOLE_HH__
#define __SIM_CONSOLE_HH__

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <readline/readline.h>
#include <readline/history.h>

#include "base/pollevent.hh"

typedef void (*console_cmd_t)(int argc, const char **argv, void *ctx);

struct ConsoleCmd {
    std::string name;
    std::string desc;
    console_cmd_t cmd;
    void *ctx;
    ConsoleCmd(std::string _name, std::string _desc, console_cmd_t _cmd, void *_ctx)
      : name(_name), desc(_desc), cmd(_cmd), ctx(_ctx) {}
};

class Console
{
  private:
    Console();
    ~Console();

    static Console *_inst;
    static Console * getInstance() {
        if (!_inst) _inst = new Console();
        return _inst;
    }

    std::map<std::string, ConsoleCmd *> commands;

    bool b_active;

  public:

    static void regCmd(const char *name, console_cmd_t cmd, void *ctx,
                       const char *desc);
    static void active();
    static void suspend();

  private:
    class Event : public PollEvent
    {
      private:
        Console *console;
      public:
        Event(Console *c, int fd, int e) : PollEvent(fd, e), console(c) {}
        virtual void process(int revent) { console->process(revent); }
    };
    friend class Event;
    Event *event;

    static void cb_linehandler(char *line);
    void process(int revent) { rl_callback_read_char(); }

    void cmdhandler(std::vector<std::string> &args);
    static void show_all_commands(int argc, const char **argv, void *ctx);
};

#endif    // __SIM_CONSOLE_HH__
