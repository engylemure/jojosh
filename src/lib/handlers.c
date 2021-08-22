#include "handlers.h"

int childs_in_background = 0;
pid_t *piped_childs = NULL;
int **pipes_from_child;
int amount_of_piped_childs_active = 0;
int amount_of_piped_childs = 0;
void sig_chld_handler(const int signal) {
  pid_t child_that_finished = wait(NULL);
  if (child_that_finished != -1) {
    if (childs_in_background != 0) {
      printf("[%d] %d Done\n", childs_in_background, child_that_finished);
      childs_in_background -= 1;
    }
    if (amount_of_piped_childs_active != 0) {
      int i;
      for (i = 0; i < amount_of_piped_childs; i++) {
        if (piped_childs[i] == child_that_finished) {
          int *pipe_from_child = pipes_from_child[i];
          close(pipe_from_child[1]);
          amount_of_piped_childs_active -= 1;
          if (amount_of_piped_childs_active == 0) {
          }
          break;
        }
      }
    }
  }
}

pid_t child_pgid = 0;
void sig_int_handler(const int signal) {
  if (child_pgid) {
    killpg(child_pgid, signal);
    if (amount_of_piped_childs) {
      free(piped_childs);
      piped_childs = NULL;
      int j;
      for (j = 0; j < amount_of_piped_childs; j++) {
        close(pipes_from_child[j][1]);
        free(pipes_from_child[j]);
      }
      free(pipes_from_child);
      pipes_from_child = NULL;
      amount_of_piped_childs = 0;
    }
  }
  //   print_weird();
}

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code) {
  if (res->is_parent) {
    printf("Unknown command %s\n", res->program_name);
  } else {
    *should_continue = false;
    *status_code = UnknownCommand;
  }
}

pid_t basic_cmd_handler(ExecArgs *exec_args, bool should_wait,
                        bool *should_continue, int *status_code) {
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

void sequential_cmd_handler(CallGroup *call_group, bool *should_continue,
                            int *status_code) {
  int i;
  for (i = 0; i < call_group->exec_amount; i++) {
    basic_cmd_handler(call_group->exec_arr[i], true, should_continue,
                      status_code);
  }
}

void parallel_cmd_handler(CallGroup *call_group, bool *should_continue,
                          int *status_code) {
  int exec_amount = call_group->exec_amount;
  pid_t child_pids[exec_amount];
  int i;
  for (i = 0; i < exec_amount; i++) {
    child_pids[i] = basic_cmd_handler(call_group->exec_arr[i], false,
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

void piped_cmd_handler(CallGroup *call_group, bool *should_continue,
                       int *status_code) {
  int exec_amount = call_group->exec_amount;
  pid_t *child_pids = malloc(sizeof(pid_t) * exec_amount);
  int **pipes =
      malloc(sizeof(int *) * (exec_amount - 1)); // [exec_amount - 1][2];
  {
    int i;
    for (i = 0; i < exec_amount - 1; i++) {
      pipes[i] = malloc(sizeof(int) * 2);
      pipe(pipes[i]);
    }
  }
  piped_childs = child_pids;
  pipes_from_child = pipes;
  amount_of_piped_childs = amount_of_piped_childs_active = exec_amount - 1;
  int i;
  for (i = 0; i < exec_amount; i++) {
    ExecArgs *exec_args = call_group->exec_arr[i];
    pid_t child_pid = fork();
    child_pids[i] = child_pid;
    if (child_pid == -1) {
      fprintf(stderr, "fork failed!\n");
    } else if (child_pid > 0) {
      setpgid(child_pid, child_pids[0]);
    } else {
      char *info = exec_args->fmt(exec_args);
      if (i > 0) {
        close(pipes[i - 1][1]);
        dup2(pipes[i - 1][0], 0);
      }
      if (i < exec_amount - 1) {
        dup2(pipes[i][1], 1);
      } else {
        dup2(1, 1);
      }
      exec_args->call(exec_args, false, false);
      *should_continue = false;
      *status_code = UnknownCommand;
      break;
    }
    if (i < exec_amount - 2 && i >= 0) {
      close(pipes[i][1]);
    }
  }
  child_pgid = child_pids[0];
  if (call_group->exec_amount > 1) {
    pid_t child_to_wait = child_pids[exec_amount - 1];
    waitpid(child_to_wait, NULL, WUNTRACED);
  }
  child_pgid = 0;
}

const char *weird_msg = "\nI feel weeird...\n";

void print_weird() { printf("%s", weird_msg); }