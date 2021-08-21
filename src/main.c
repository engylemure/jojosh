#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "lib/lib.h"
#include "lib/util/string_util/string_util.h"

void unknown_cmd_info(CallResult *res, bool *should_continue, int *status_code) {
  if (res->is_parent) {
    printf("Unknown command %s\n", res->program_name);
  } else {
    *should_continue = false;
    *status_code = UnknownCommand;
  }
}

pid_t basic_handle(ExecArgs *exec_args, bool should_wait, bool *should_continue, int *status_code) {
  CallResult *res = exec_args->call(exec_args, should_wait);
  switch (res->status) {
  case Continue:
    break;
  case Exit:
    *should_continue = false;
    break;
  case UnknownCommand:
    unknown_cmd_info(res, should_continue, status_code);
    break;
  }
  pid_t child_pid = res->child_pid;
  res->drop(res);
  return child_pid;
}

void sequential_handle(CallGroup *call_group, bool *should_continue,
                       int *status_code) {
  int i;
  for (i = 0; i < call_group->exec_amount; i++) {
    basic_handle(call_group->exec_arr[i], true, should_continue, status_code);
  }
}


pid_t child_pgid = 0;

void sig_int_handler(const int signal) {
  if (child_pgid) {
    killpg(child_pgid, signal);
  } 
}

void parallel_handle(CallGroup *call_group, bool *should_continue, int* status_code ){
  pid_t child_pids[call_group->exec_amount];
  int i;
  for (i = 0; i < call_group->exec_amount; i++) {
    child_pids[i] = basic_handle(call_group->exec_arr[i], false, should_continue, status_code);
    setpgid(child_pids[i], child_pids[0]);
  }
  child_pgid = child_pids[0];
  while(wait(NULL) != -1);
  child_pgid = 0;
}

int main(void) {
  ShellState *state = initialize_shell_state();
  signal(SIGINT, sig_int_handler);
  signal(SIGQUIT, sig_int_handler);
  bool should_continue = true;
  int status_code = 0;
  while (should_continue) {
    CallArg *call_arg = prompt_user(state);
    CallGroups *call_groups = call_arg->call_groups(call_arg);
    int i;
    for (i = 0; i < call_groups->len; i++) {
      CallGroup *call_group = call_groups->groups[i];
      switch (call_group->type) {
      case Basic:
        if (call_group->exec_amount) basic_handle(call_group->exec_arr[0], true, &should_continue, &status_code);
        break;
      case Parallel:
        parallel_handle(call_group, &should_continue, &status_code);
        break;
      case Sequential:
        sequential_handle(call_group, &should_continue, &status_code);
        break;
      case Piped:
        todo("PIPED commands!");
      default:
        break;
      }
    }
    call_groups->drop(call_groups);
    call_arg->drop(call_arg);
  }
  state->drop(state);
  return status_code;
}