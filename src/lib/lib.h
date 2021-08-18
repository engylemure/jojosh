#ifndef LIB_JOJOSH_H
#define LIB_JOJOSH_H

#include <stdbool.h>
#include <stdio.h>

enum CallType {
  Basic,
  Parallel,
  Piped,
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
  void (*drop)(struct callResult *self);
} CallResult;

typedef struct callArgs {
  int call_amount;
  enum CallType type;
  CallResult *(*call)(struct callArgs *self);
  void (*add_arg)(struct callArgs *self, char *str_call);
  void (*drop)(struct callArgs *self);
  char **str_calls;
} CallArgs;

CallArgs *prompt_user(ShellState *state);
ShellState *initialize_shell_state();
void install_sig_handlers();
void clean_sig_handlers();

#endif