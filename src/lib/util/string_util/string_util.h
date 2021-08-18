#ifndef LIB_STRING_UTIL_H
#define LIB_STRING_UTIL_H

#include "../linked_list/linked_list.h"
#include <stdbool.h>

#define BUFFER_MAX_SIZE 1024
/*
    Code for more easily handle string manipulation.
    author: Jordao Rosario <jordao.rosario01@gmail.com>
*/
typedef struct strSplitRes {
  LinkedList *str_list;
  char *src_str;
  void (*drop)(struct strSplitRes *res);
} StrSplitResult;

StrSplitResult *str_split(char *str, char *delim);
StrSplitResult *create_split_res();
void drop_str_split_res(StrSplitResult *res);

char *str_trim(char *str);
char *fmt_str(void *data);
bool str_equals(char* self, char* other);

#endif