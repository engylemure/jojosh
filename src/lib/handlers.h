#ifndef LIB_HANDLERS_H
#define LIB_HANDLERS_H

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib.h"
#include "util/string_util/string_util.h"

void print_weird();

/*
 * Signal Handlers
 */
void sig_int_handler(const int signal);
void sig_chld_handler(const int signal);

/*
 * Commands handlers
 */ 
pid_t basic_cmd_handler(ExecArgs *exec_args, bool should_wait, bool *should_continue,
                   int *status_code);
void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code);

void sequential_cmd_handler(CallGroup *call_group, bool *should_continue,
                       int *status_code);

void parallel_cmd_handler(CallGroup *call_group, bool *should_continue,
                     int *status_code);

void piped_cmd_handler(CallGroup *call_group, bool *should_continue,
                  int *status_code);


#endif