#include "sim/async.hh"
#include "sim/console.hh"

static void gem5_cmd_kill(int argc, const char **argv, void *ctx)
{
    async_event = true;
    async_exit = true;
}

static void gem5_cmd_clear(int argc, const char **argv, void *ctx)
{
    if (system("clear")) {};
}

Console * Console::_inst = NULL;

Console::Console()
    : b_active(false)
{
    event = new Event(this, STDIN_FILENO, POLLIN);

    // register builtin command
    commands["clear"] = new ConsoleCmd("clear", "clear screen",
                                      gem5_cmd_clear, NULL);
    commands["kill"]  = new ConsoleCmd("kill", "kill simulation and exit",
                                      gem5_cmd_kill, NULL);
    commands["help"]  = new ConsoleCmd("help", "show all available commands",
                                      Console::show_all_commands, (void *)this);
}

Console::~Console()
{
    if (event)
        delete event;

    for (std::map<std::string, ConsoleCmd *>::iterator it = commands.begin();
         it != commands.end();
         it ++)
    {
        delete it->second;
    }
}

void
Console::regCmd(const char *name, console_cmd_t cmd, void *ctx,
                const char  *desc)
{
    Console *console = getInstance();
    if (console->commands.find(name) != console->commands.end())
        panic("duplicate console command: %s.\n", name);
    console->commands[name] = new ConsoleCmd(name, desc, cmd, ctx);
}

void
Console::active()
{
    Console *console = getInstance();
    if (!console->b_active) {
        printf("Enter interactive console mode.\n");
        // install the line handler
        rl_callback_handler_install ("(gem5) ", Console::cb_linehandler);
        // schedule keyboard event
        pollQueue.schedule(console->event);
        console->b_active = true;
    }
}

void
Console::suspend()
{
    Console *console = getInstance();
    if (console->b_active) {
        // deschedule keyboard event
        pollQueue.remove(console->event);
        console->b_active = false;
        // remove the line handler
        rl_callback_handler_remove ();
        printf("Exit interactive console mode.\n");
    }
}

void Console::cb_linehandler(char *line)
{
    if (line == NULL || strcmp (line, "exit") == 0) {
        if (line == 0)
            printf ("\n");
        Console::suspend();
    }
    else {
        if (*line) {
            add_history (line);

            std::vector<std::string> args;
            char *saveptr1 = NULL;
            for (char *p=line; ; p=NULL) {
                char *token = strtok_r(p, " ", &saveptr1);
                if (NULL == token)
                    break;
                args.push_back(token);
            }
            Console::getInstance()->cmdhandler(args);
        }

        free (line);
    }
}

void
Console::cmdhandler(std::vector<std::string> &args)
{
    for (std::map<std::string, ConsoleCmd *>::iterator it = commands.begin();
         it != commands.end();
         it ++)
    {
         if (it->first == args[0]) {
             int argc = args.size();
             const char **argv = new const char*[argc];
             for (int i=0; i<argc; i++)
                 argv[i] = args[i].c_str();
             it->second->cmd(argc, argv, it->second->ctx);
             return;
         }
    }

    printf("Unknown command: \"%s\".\n", args[0].c_str());
}

void
Console::show_all_commands(int argc, const char **argv, void *ctx)
{
    Console *console = static_cast<Console *>(ctx);
    int max_cmd_len = 0;
    char strfmt[32];

    for (std::map<std::string, ConsoleCmd *>::iterator it = console->commands.begin();
         it != console->commands.end();
         it ++)
    {
        if (it->first.length() > max_cmd_len)
            max_cmd_len = it->first.length();
    }
    snprintf(strfmt, 32, "  %%-%ds  %%s\n", max_cmd_len);

    printf("\nAvailable commands:\n\n");
    for (std::map<std::string, ConsoleCmd *>::iterator it = console->commands.begin();
         it != console->commands.end();
         it ++)
    {
        printf(strfmt, it->first.c_str(), it->second->desc.c_str());
    }
    printf("\n");
}

