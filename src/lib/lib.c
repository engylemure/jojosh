#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib.h"
#include "util/string_util/string_util.h"

char *read_env(char *name) {
  char *env = malloc(sizeof(char) * BUFFER_MAX_SIZE);
  char *env_value;
  if (!(env_value = getenv(name))) {
    fprintf(stderr, "The environment variable '%s' is not available!\n", name);
    exit(1);
  }
  if (snprintf(env, BUFFER_MAX_SIZE, "%s", env_value) >= BUFFER_MAX_SIZE) {
    fprintf(stderr, "BUFFER_MAX_SIZE wasn't enough to hold '%s' value",
            env_value);
    exit(1);
  }
  return env;
}

void drop_shell_state(ShellState *self) {
  free(self->HOME);
  free(self->pwd);
  free(self);
}

char *pretty_pwd(ShellState *self) {
  if (!strcmp(self->pwd, self->HOME)) {
    return strdup("~/");
  }
  char *pwd = strstr(self->pwd, self->HOME);
  if (pwd != NULL) {
    pwd = pwd + strlen(self->HOME);
    char *prettied_pwd = malloc(sizeof(char) * (strlen(pwd) + 2));
    prettied_pwd[0] = '~';
    prettied_pwd[1] = '\0';
    strcat(prettied_pwd, pwd);
    return prettied_pwd;
  }
  return strdup(self->pwd);
}

ShellState *initialize_shell_state() {
  char *PWD = read_env("PWD");
  char *HOME = read_env("HOME");
  ShellState *state = malloc(sizeof(ShellState));
  state->pwd = PWD;
  state->HOME = HOME;
  state->pretty_pwd = *pretty_pwd;
  state->drop = *drop_shell_state;
  return state;
}

CallResult *call(CallArgs *self);

void drop_call_args(CallArgs *self) {
  if (self != NULL) {
    int i;
    for (i = 0; i < self->call_amount; i++) {
      free(self->str_calls[i]);
    }
    free(self->str_calls);
    free(self);
  } else {
    perror("double free on CallArgs!\n");
    exit(1);
  }
}
void add_arg(CallArgs *call_args, char *arg) {
  if (call_args != NULL) {
    call_args->str_calls[call_args->call_amount] = arg;
    call_args->call_amount += 1;
  } else {
    perror("we cannot add_arg into a NULL CallArgs!\n");
    exit(1);
  }
}

CallArgs *initialize_call_args(int call_amount) {
  CallArgs *self = malloc(sizeof(CallArgs));
  self->call_amount = 0;
  self->type = Basic;
  self->call = *call;
  self->str_calls = malloc(sizeof(char *) * call_amount);
  self->add_arg = *add_arg;
  self->drop = *drop_call_args;
  return self;
}

const char BlueAnsi[] = "\033[94m";
const char JojoAnsi[] = "\033[95m";
const char EndAnsi[] = "\033[0m";

CallArgs *prompt_user(ShellState *state) {
  if (state != NULL) {
    char input[BUFFER_MAX_SIZE];
    fflush(stdout);
    char *pwd = state->pretty_pwd(state);
    printf("%s%s > %s%sJoJo%s%s > %s", BlueAnsi, pwd, EndAnsi, JojoAnsi,
           EndAnsi, BlueAnsi, EndAnsi);
    free(pwd);
    fflush(stdin);
    fgets(input, BUFFER_MAX_SIZE, stdin);
    input[strcspn(input, "\n")] = '\0';
    CallArgs *call = initialize_call_args(1);
    call->add_arg(call, strdup(input));
    return call;
  } else {
    perror("provided ShellState is NULL\n");
    exit(1);
  }
}

void install_sig_handlers() {
  // TODO
}

void clean_sig_handlers() {
  // TODO
}

typedef struct execArgs {
  int argc;
  char **argv;
  void (*drop)(struct execArgs *self);
} ExecArgs;

void drop_exec_args(ExecArgs *self);

// We want to parse and tokenize to separate this
// in different call objects instead of this ExecArgs, this actually should be
// the final result of the parsing instead of the initial one
ExecArgs *from_str_call(char *str) {
  ExecArgs *self = malloc(sizeof(ExecArgs));
  self->drop = *drop_exec_args;
  StrSplitResult *split_res = str_split(strdup(str), " ");
  self->argc = split_res->str_list->count(split_res->str_list);
  ListNode *node;
  self->argv = malloc(sizeof(char *) * (self->argc + 1));
  int i;
  for (node = split_res->str_list->head, i = 0; node != NULL;
       node = next_node(node)) {
    if (strlen(node->data) != 0) {
      char *trimmed_cmd = str_trim(node->data);
      self->argv[i++] = trimmed_cmd;
    }
  }
  self->argv[i] = NULL;
  self->argc = i;
  split_res->drop(split_res);
  return self;
}

void drop_exec_args(ExecArgs *self) {
  int i;
  for (i = 0; i < self->argc; i++) {
    free(self->argv[i]);
  }
  free(self->argv);
  free(self);
}

enum CallStatus basic_call(CallArgs *call_args, CallResult *res) {
  char *delim = " ";
  ExecArgs *exec_args = from_str_call(call_args->str_calls[0]);
  enum CallStatus status = UnknownCommand;
  if (exec_args->argc == 0) {
    status = Continue;
  } else {
    char *program_name = exec_args->argv[0];
    if (str_equals(program_name, "exit")) {
      printf("JoJo Shell was exited!\n");
      status = Exit;
    } else if (str_equals(program_name, "cd")) {
      // TODO
      printf("TODO: handle 'cd' call\n");
      status = Continue;
    } else {
      pid_t child_pid = fork();
      if (child_pid == -1) {
        perror("We can't start a new program since 'fork' failed!\n");
        exit(1);
      } else if (child_pid) {
        status = Continue;
        int wait_status;
        waitpid(child_pid, &wait_status, WUNTRACED);
        if (WIFEXITED(wait_status) &&
            (WEXITSTATUS(wait_status) == UnknownCommand)) {
          status = UnknownCommand;
        }
      } else {
        res->is_parent = false;
        execvp(program_name, exec_args->argv);
      }
    }
  }
  exec_args->drop(exec_args);
  return status;
}

void drop_call_res(CallResult *self) { free(self); }

CallResult *call(CallArgs *call_args) {
  CallResult *res = malloc(sizeof(CallResult));
  res->is_parent = true;
  res->drop = *drop_call_res;
  switch (call_args->type) {
  case Basic:
    res->status = basic_call(call_args, res);
    break;
  default:
    res->status = UnknownCommand;
  }
  if (res->status == UnknownCommand) {
    res->program_name = call_args->str_calls[0];
  }
  return res;
}