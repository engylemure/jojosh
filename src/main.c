#include <stdbool.h>
#include <stdio.h>

#include "lib/lib.h"

int main(void) {
  ShellState *state = initialize_shell_state();
  install_sig_handlers();
  bool should_continue = true;
  int status_code = 0;
  while (should_continue) {
    CallArgs *args = prompt_user(state);
    CallResult *res = args->call(args);
    switch (res->status) {
    case Exit:
      should_continue = false;
      break;
    case Continue:
      break;
    case UnknownCommand:
      if (res->is_parent) {
          printf("Unknown command %s\n", res->program_name);
      } else {
          should_continue = false;
          status_code = UnknownCommand;
      }
      break;
    }
    res->drop(res);
    args->drop(args);
  }
  state->drop(state);
  return status_code;
}