#include "string_util.h"
#include "../linked_list/linked_list.h"
#include <ctype.h>
#include <string.h>

StrSplitResult *create_split_res() {
  StrSplitResult *res = malloc(sizeof(StrSplitResult));
  res->str_list = initialize_list(fmt_str, NULL);
  res->drop = *drop_str_split_res;
  return res;
}

void drop_str_split_res(StrSplitResult *res) {
  if (res != NULL) {
    res->str_list->drop(res->str_list);
    free(res->src_str);
    res->src_str = NULL;
    free(res);
  } else {
    perror("StrSplitResult drop_str_split_res on NULL value\n");
  }
}

StrSplitResult *str_split(char *str, char *delim) {
  StrSplitResult *res = create_split_res();
  res->src_str = str;
  char *str_ptr = strtok(str, delim);
  while (str_ptr != NULL) {
    res->str_list->push(res->str_list, str_ptr);
    str_ptr = strtok(NULL, delim);
  }
  return res;
}

char *fmt_str(void *data) {
  char *str = malloc(strlen(data) + 1);
  strcpy(str, data);
  return str;
}

int get_first_position(char *str) {
  int i = 0;
  while (isspace(str[i])) {
    i += 1;
  }
  return (i);
}

int get_last_position(char *str) {
  int i = strlen(str) - 1;
  while (isspace(str[i])) {
    i -= 1;
  }
  return (i);
}

char *str_trim(char *str) {
  char *trim = NULL;
  int i, len, start, end;
  if (str != NULL) {
    start = get_first_position(str);
    end = get_last_position(str);
    len = (get_last_position(str) - get_first_position(str)) + 2; // the + 1 is for the termination of the string
    trim = malloc(sizeof(char)*len);
    i = 0;
    while (start <= end) {
      char c = (unsigned char)str[start];
      start++;
      trim[i++] = c;
    }
    trim[i] = '\0';
  }
  return (trim);
}

bool str_equals(char* self, char* other) {
    return strcmp(self, other) == 0;
}