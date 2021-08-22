#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib/lib.h"
#include "lib/util/string_util/string_util.h"

char *weird_msg = "\nI feel weeird...\n";

void print_weird() { printf("%s", weird_msg); }

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code) {
  if (res->is_parent) {
    printf("Unknown command %s\n", res->program_name);
  } else {
    *should_continue = false;
    *status_code = UnknownCommand;
  }
}

pid_t basic_handle(ExecArgs *exec_args, bool should_wait, bool *should_continue,
                   int *status_code) {
  CallResult *res = exec_args->call(exec_args, true, should_wait);
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
  print_weird();
}

int childs_in_background = 0;
void sig_chld_handler(const int signal) {
  pid_t child_that_finished = wait(NULL);
  if (child_that_finished != -1) {

    if (childs_in_background != 0) {
      printf("[%d] %d Done\n", childs_in_background, child_that_finished);
      print_weird();
      childs_in_background -= 1;
    }
  }
}

void parallel_handle(CallGroup *call_group, bool *should_continue,
                     int *status_code) {
  int exec_amount = call_group->exec_amount;
  pid_t child_pids[exec_amount];
  int i;
  for (i = 0; i < exec_amount; i++) {
    child_pids[i] = basic_handle(call_group->exec_arr[i], false,
                                 should_continue, status_code);
    if (i < exec_amount - 1) {
      childs_in_background += 1;
      printf("[%d] %d\n", childs_in_background, child_pids[i]);
    }
    setpgid(child_pids[i], child_pids[0]);
  }
  child_pgid = child_pids[0];
  if (call_group->exec_amount > 1) {
    pid_t child_to_wait = child_pids[exec_amount - 1];
    waitpid(child_to_wait, NULL, WUNTRACED);
  }
  child_pgid = 0;
}

// void piped_call(ExecArgs **exec_arr, int exec_amount, int idx,
//                 int last_stdout, bool *should_continue) {
//   pid_t child_pid = fork();
//   if (child_pid == -1) {
//     printf("error forking!");
//   }
//   if (idx == 0) {
//     child_pgid = child_pid;
//   }
//   if (child_pid == 0) {
//     if (last_stdout != -1) {
//       dup2(last_stdout, 0);
//     }
//     int fd[2];
//     if (pipe(fd) == 0) {
//       printf("[%d, %d]\n", fd[0], fd[1]);
//       ExecArgs *exec_arg = exec_arr[idx];
//       if (idx < exec_amount - 1) {
//         dup2(fd[1], 1);
//       }
//       if (idx < exec_amount) {
//         piped_call(exec_arr, exec_amount, idx + 1, fd[0], should_continue);
//       }
//       exec_arg->call(exec_arg, false, false);
//       should_continue = false;
//     }
//   }
// }

void piped_handle(CallGroup *call_group, bool *should_continue,
                  int *status_code) {
  int exec_amount = call_group->exec_amount;
  pid_t child_pids[exec_amount];
  int pipes[exec_amount][2];
  int i;
  for (i = 0; i < exec_amount; i++) {
    ExecArgs *exec_args = call_group->exec_arr[i];
    pipe(pipes[i]);
    pid_t child_pid = fork(); 
    child_pids[i] = child_pid;
    if (child_pid > 0) {
      setpgid(child_pid, child_pids[0]);
    }
    if (child_pid == -1) {
      fprintf(stderr, "fork failed!");
    }
    if (child_pid == 0) {
      char* info = exec_args->fmt(exec_args);
      if (i > 0) {
        dup2(pipes[i - 1][0], 0);
      }
      if (i < exec_amount -1) {
        dup2(pipes[i][1], 1);
      } else {
        dup2(1, 1);
      }
      exec_args->call(exec_args, false, false);
      *should_continue = false;
      break;
    }
  }
  
}