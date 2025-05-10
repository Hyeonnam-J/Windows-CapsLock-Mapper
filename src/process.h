#ifndef PROCESS_H
#define PROCESS_H

int on_runner();
int off_runner();
int show_status();
int add_env();
int remove_env();
int add_registry();
int remove_registry();
int show_version();
int show_help();
int show_help_invalid();

typedef int (*ProcessHandler)();

struct Option {
    char* name;
    char* alias;
    ProcessHandler handler;
};

struct Command {
    char* name;
    ProcessHandler handler;
};

struct CommandWithOptions {
    struct Command command;
    struct Option options[3];
};

extern struct CommandWithOptions commandWithOptions[];

#endif