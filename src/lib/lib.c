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

void drop_call_args(CallArg *self) {
  if (self != NULL) {
    free(self->arg);
    free(self);
  } else {
    perror("double free on CallArgs!\n");
    exit(1);
  }
}

CallGroups *call_groups(CallArg *call_arg);

CallArg *initialize_call_args(char *arg) {
  CallArg *self = malloc(sizeof(CallArg));
  self->arg = strdup(arg);
  self->call_groups = *call_groups;
  self->drop = *drop_call_args;
  return self;
}

const char BlueAnsi[] = "\033[94m";
const char JojoAnsi[] = "\033[95m";
const char EndAnsi[] = "\033[0m";
const char RedAnsi[] = "\033[91m";
const char YellowAnsi[] = "\033[93m";

void todo(char *msg) { printf("%sTODO: %s%s\n", YellowAnsi, msg, EndAnsi); }

CallArg *prompt_user(ShellState *state) {
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
    CallArg *call = initialize_call_args(input);
    return call;
  } else {
    perror("provided ShellState is NULL\n");
    exit(1);
  }
}

void install_sig_handlers() { todo("Install signal handlers!"); }

void clean_sig_handlers() { todo("Clean Signal handlers!"); }

void drop_exec_args(ExecArgs *self);

char *fmt_call(void *call) {
  char *call_str = (char *)call;
  int fmt_len = strlen(call_str) + 3;
  char *fmt = malloc(fmt_len);
  fmt[0] = '\'';
  fmt[1] = '\0';
  strcat(fmt, call_str);
  fmt[fmt_len - 2] = '\'';
  fmt[fmt_len - 1] = '\0';
  return fmt;
}

char *fmt_call_list(void *call_list) {
  LinkedList *list = (LinkedList *)call_list;
  return list->fmt(list);
}

LinkedList *new_list_call() { return initialize_list(&fmt_call, free); }

char *fmt_exec_arg(void *data);

char *fmt_call_group(void *data) {
  CallGroup *call_group = data;
  char *formatted_call_group = malloc(sizeof(char) * BUFFER_MAX_SIZE / 2);
  formatted_call_group[0] = '[';
  formatted_call_group[1] = '\0';
  int i;
  for (i = 0; i < call_group->exec_amount; i++) {
    char *exec_arg = fmt_exec_arg(call_group->exec_arr[i]);
    if (i == 0) {
      strcat(formatted_call_group, exec_arg);
    } else {
      strcat(formatted_call_group, ",");
      strcat(formatted_call_group, exec_arg);
    }
    free(exec_arg);
  }
  strcat(formatted_call_group, "](");
  char *call_group_type;
  switch (call_group->type) {
  case Basic:
    call_group_type = "Basic";
    break;
  case Parallel:
    call_group_type = "Parallel";
    break;
  case Piped:
    call_group_type = "Piped";
    break;
  case RedirectStdIn:
    call_group_type = "RedirectStdin";
    break;
  case RedirectStdout:
    call_group_type = "RedirectStdout";
    break;
  case Sequential:
    call_group_type = "Sequential";
    break;
  }
  strcat(formatted_call_group, call_group_type);
  strcat(formatted_call_group, ")");
  return formatted_call_group;
}

void drop_call_group(CallGroup *self) {
  int i;
  for (i = 0; i < self->exec_amount; i++) {
    self->exec_arr[i]->drop(self->exec_arr[i]);
  }
  free(self->exec_arr);
  self->exec_arr = NULL;
  if (self->file_name != NULL) {
    free(self->file_name);
    self->file_name = NULL;
  }
  free(self);
}

LinkedList *new_list_call_group() {
  return initialize_list(&fmt_call_group, (void (*)(void *))drop_call_group);
}

char *fmt_exec_arg(void *data) {
  ExecArgs *exec_args = data;
  int argv_len = 15;
  int i;
  for (i = 0; i < exec_args->argc; i++) {
    argv_len += strlen(exec_args->argv[i]) + 1;
  }
  char *argv = malloc(sizeof(char) * argv_len);
  argv[0] = '[';
  argv[1] = '\0';
  for (i = 0; i < exec_args->argc; i++) {
    char *formatted_call = fmt_call(exec_args->argv[i]);
    strcat(argv, formatted_call);
    free(formatted_call);
    if (i != exec_args->argc - 1)
      strcat(argv, ",");
  }
  strcat(argv, "]");
  return argv;
}

LinkedList *new_list_exec_args() {
  return initialize_list(&fmt_exec_arg, (void (*)(void *))drop_call_args);
}

LinkedList *calls_from_str(char *str) {
  StrSplitResult *split_res = str_split(strdup(str), " ");
  LinkedList *list_calls =
      initialize_list(&fmt_call_list, (void (*)(void *))drop_list);
  LinkedList *list_call = initialize_list(&fmt_call, free);
  int i;
  ListNode *str_node;
  for (str_node = split_res->str_list->head, i = 0; str_node != NULL;
       str_node = split_res->str_list->next_node(str_node)) {
    if (strlen(str_node->data) != 0) {
      char *trimmed_cmd = str_trim(str_node->data);
      if (str_equals(trimmed_cmd, "|")) {
        list_calls->push(list_calls, list_call);
        list_call = initialize_list(&fmt_call, free);
      } else if (str_equals(trimmed_cmd, "&")) {
        list_calls->push(list_calls, list_call);
        list_call = initialize_list(&fmt_call, free);
      } else if (str_equals(trimmed_cmd, "&&")) {
        list_calls->push(list_calls, list_call);
        list_call = initialize_list(&fmt_call, free);
      } else {
        list_call->push(list_call, trimmed_cmd);
      }
    }
  }
  list_calls->push(list_calls, list_call);
  list_calls->print(list_calls, stdout);
  split_res->drop(split_res);
  return list_calls;
}

ExecArgs *from_str_call(char *str) {
  calls_from_str(str);
  ExecArgs *self = malloc(sizeof(ExecArgs));
  self->drop = *drop_exec_args;
  StrSplitResult *split_res = str_split(strdup(str), " ");
  split_res->str_list->print(split_res->str_list, stdout);
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

ExecArgs *from_call_list(LinkedList *list) {
  ExecArgs *self = malloc(sizeof(ExecArgs));
  self->drop = *drop_exec_args;
  self->argc = list->count(list);
  ListNode *node;
  self->argv = malloc(sizeof(char *) * (self->argc + 1));
  int i;
  for (node = list->head, i = 0; node != NULL; node = next_node(node)) {
    if (strlen(node->data) != 0) {
      char *trimmed_cmd = str_trim(node->data);
      self->argv[i++] = trimmed_cmd;
    }
  }
  self->argv[i] = NULL;
  self->argc = i;
  list->drop(list);
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

enum CallStatus basic_call(CallArg *call_arg, CallResult *res) {
  char *delim = " ";
  LinkedList *calls_list = calls_from_str(call_arg->arg);
  ExecArgs *exec_args = from_call_list(calls_list->head->data);
  calls_list->drop(calls_list);
  enum CallStatus status = UnknownCommand;
  if (exec_args->argc == 0) {
    status = Continue;
  } else {
    char *program_name = exec_args->argv[0];
    if (str_equals(program_name, "exit")) {
      printf("JoJo Shell was exited!\n");
      status = Exit;
    } else if (str_equals(program_name, "cd")) {
      todo("handle 'cd' call");
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

void drop_call_res(CallResult *self) {
  if (self->program_name != NULL) {
    free(self->program_name);
    self->program_name = NULL;
  }
  free(self);
}

CallGroup *call_group(LinkedList *list_exec_args, enum CallType type) {
  CallGroup *self = malloc(sizeof(CallGroup));
  int count = list_exec_args->count(list_exec_args);
  ExecArgs **exec_arr = malloc(sizeof(ExecArgs *) * count);
  ExecArgs *exec_args;
  int i = 0;
  while ((exec_args = list_exec_args->deque(list_exec_args)) != NULL) {
    exec_arr[i++] = exec_args;
  }
  list_exec_args->drop(list_exec_args);
  self->type = type;
  self->exec_amount = count;
  self->exec_arr = exec_arr;
  self->drop = *drop_call_group;
  self->file_name = NULL;
  return self;
}

void drop_call_groups(CallGroups *self) {
  int i;
  for (i = 0; i < self->len; i++) {
    CallGroup *call_group = self->groups[i];
    call_group->drop(call_group);
  }
  free(self->groups);
  free(self);
}
// Temporary handling unwanted or unused characters
bool is_ignorable_call(char *str) {
  return strlen(str) || str[0] == '(' || str[0] == ')';
}

// This is a basic tokenizer to more easily handle the
// CallGroup's since it's a dumb one at the moment will not support
// different call types at the same time
CallGroups *call_groups(CallArg *call_arg) {
  // Splitting input by space
  StrSplitResult *split_res = str_split(strdup(call_arg->arg), " ");
  LinkedList *list_call_group = new_list_call_group();
  LinkedList *list_exec_args = new_list_exec_args();
  LinkedList *list_call = new_list_call();
  enum CallType type = Basic;
  int i;
  ListNode *str_node;
  for (str_node = split_res->str_list->head, i = 0; str_node != NULL;
       str_node = split_res->str_list->next_node(str_node)) {
    if (is_ignorable_call(str_node->data)) {
      char *trimmed_cmd = str_trim(str_node->data);
      if (str_equals(trimmed_cmd, "|")) {
        ExecArgs *exec_arg = from_call_list(list_call);
        list_call = new_list_call();
        if (type == Basic || type == Parallel) {
          type = Parallel;
          list_exec_args->push(list_exec_args, exec_arg);
        } else {
          list_call_group->push(list_call_group,
                                call_group(list_exec_args, type));
          list_exec_args = new_list_exec_args();
          type = Basic;
        }
        free(trimmed_cmd);
      } else if (str_equals(trimmed_cmd, "&")) {
        ExecArgs *exec_arg = from_call_list(list_call);
        list_call = new_list_call();
        if (type == Basic || type == Piped) {
          type = Piped;
          list_exec_args->push(list_exec_args, exec_arg);
        } else {
          list_call_group->push(list_call_group,
                                call_group(list_exec_args, type));
          list_exec_args = new_list_exec_args();
          type = Basic;
        }
        free(trimmed_cmd);
      } else if (str_equals(trimmed_cmd, "&&")) {
        ExecArgs *exec_arg = from_call_list(list_call);
        list_call = new_list_call();
        if (type == Basic || type == Sequential) {
          type = Sequential;
          list_exec_args->push(list_exec_args, exec_arg);
        } else {
          list_call_group->push(list_call_group,
                                call_group(list_exec_args, type));
          list_exec_args = new_list_exec_args();
          type = Basic;
        }
        free(trimmed_cmd);
      } else {
        list_call->push(list_call, trimmed_cmd);
      }
    }
  }
  if (list_call->count(list_call)) {
    list_exec_args->push(list_exec_args, from_call_list(list_call));
  } else {
    list_call->drop(list_call);
  }
  list_call_group->push(list_call_group, call_group(list_exec_args, type));
  list_call_group->print(list_call_group, stdout);
  int call_group_count = list_call_group->count(list_call_group);
  CallGroups *self = malloc(sizeof(CallGroups));
  self->drop = *drop_call_groups;
  self->groups = malloc(sizeof(CallGroups *) * call_group_count);
  self->len = call_group_count;
  i = 0;
  CallGroup *call_group;
  while ((call_group = list_call_group->deque(list_call_group)) != NULL) {
    self->groups[i++] = call_group;
  }
  list_call_group->drop(list_call_group);
  split_res->drop(split_res);
  return self;
}

CallResult *new_call_result(enum CallStatus status, bool is_parent,
                            char *program_name, pid_t child_pid) {
  CallResult *res = malloc(sizeof(CallResult));
  res->drop = *drop_call_res;
  res->is_parent = is_parent;
  res->status = status;
  res->program_name = program_name;
  res->child_pid = child_pid;
  return res;
}

CallResult *basic_exec_args_call(ExecArgs *exec_args, bool should_wait) {
  enum CallStatus status = UnknownCommand;
  char *program_name = NULL;
  bool is_parent = true;
  pid_t child_pid = 0;
  if (exec_args->argc == 0) {
    status = Continue;
  } else {
    program_name = exec_args->argv[0];
    if (str_equals(program_name, "exit")) {
      printf("JoJo Shell was exited!\n");
      status = Exit;
    } else if (str_equals(program_name, "cd")) {
      todo("handle 'cd' call");
      status = Continue;
    } else {
      child_pid = fork();
      if (child_pid == -1) {
        perror("We can't start a new program since 'fork' failed!\n");
        exit(1);
      } else if (child_pid) {
        status = Continue;
        if (should_wait) {
          int wait_status;
          waitpid(child_pid, &wait_status, WUNTRACED);
          if (WIFEXITED(wait_status) &&
              (WEXITSTATUS(wait_status) == UnknownCommand)) {
            status = UnknownCommand;
          }
        }
      } else {
        execvp(program_name, exec_args->argv);
        is_parent = false;
      }
    }
  }
  return new_call_result(status, is_parent,
                         status == UnknownCommand ? strdup(program_name) : NULL,
                         child_pid);
}

// CallResult *call(CallArg *call_args) {
//   CallResult *res = malloc(sizeof(CallResult));
//   res->is_parent = true;
//   res->drop = *drop_call_res;
//   res->program_name = NULL;
//   switch (call_args->type) {
//   case Basic:
//     res->status = basic_call(call_args, res);
//     break;
//   default:
//     res->status = UnknownCommand;
//   }
//   if (res->status == UnknownCommand) {
//     res->program_name = strdup(call_args->arg);
//   }
//   return res;
// }