#ifndef LIB_JOJOSH_H
#define LIB_JOJOSH_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

enum CallType {
  Basic,
  Sequential,
  Parallel,
  Piped,
  RedirectStdout,
  RedirectStdIn,
};

typedef struct shellState {
  char *pwd;
  char *HOME;
  char *(*pretty_pwd)(struct shellState *state);
  void (*drop)(struct shellState *state);
} ShellState;

enum CallStatus { Continue, Exit, UnknownCommand };

typedef struct callResult {
  char *program_name;
  enum CallStatus status;
  bool is_parent;
  pid_t child_pid;
  void (*drop)(struct callResult *self);
} CallResult;

typedef struct execArgs {
  int argc;
  char **argv;
  void (*drop)(struct execArgs *self);
  char* (*fmt)(struct execArgs *self);
  CallResult *(*call)(struct execArgs* self, bool should_fork, bool should_wait);
} ExecArgs;

typedef struct callGroup {
  int exec_amount;
  enum CallType type;
  char* file_name;
  ExecArgs** exec_arr;
  void (*drop)(struct callGroup *self);
} CallGroup;

typedef struct callGroups {
  int len;
  CallGroup** groups;
  void (*drop)(struct callGroups *self);
} CallGroups;

typedef struct callArg {
  CallGroups *(*call_groups)(struct callArg *self);
  void (*drop)(struct callArg *self);
  char *arg;
} CallArg;

CallArg *prompt_user(ShellState *state);
ShellState *initialize_shell_state();
void todo(char *msg);
void debug_lib(bool should_debug);

#endif