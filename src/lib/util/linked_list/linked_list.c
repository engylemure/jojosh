#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ListNode *initialize_list_node(void *data) {
  ListNode *new_node = malloc(sizeof(ListNode));
  new_node->previous = NULL;
  new_node->next = NULL;
  new_node->data = data;
  return new_node;
}

int cmp(const void *self, const void *other);
int delete_node(LinkedList *self, void *to_be_deleted);
void push(LinkedList *self, void *to_be_pushed);
void *pop(LinkedList *self);
void drop(LinkedList *self);
void print_list(LinkedList *self, FILE *stream);
void print_node(LinkedList *list, void *node, FILE *stream);
void iterate(LinkedList *list, void (*cb)(void *data, int i, void *aux_arg),
             void *aux_arg);
ListNode* next_node(ListNode* node);
char* fmt_list(LinkedList* list);
int count(LinkedList *list);
void* deque(LinkedList *list);
char *default_fmt_node_data(void *data);

LinkedList *initialize_list(char *(*fmt_data)(void *to_be_printed),
                            void (*drop_data)(void *data)) {
  LinkedList *new_list = (LinkedList *)malloc(sizeof(LinkedList));
  new_list->head = NULL;
  new_list->tail = NULL;
  new_list->next_node = *next_node;
  new_list->count = *count;
  new_list->print = *print_list;
  new_list->print_node = *print_node;
  new_list->cmp = *cmp;
  new_list->delete_node = *delete_node;
  new_list->push = *push;
  new_list->pop = *pop;
  new_list->drop = *drop;
  new_list->drop_data = drop_data;
  new_list->for_each = *iterate;
  new_list->fmt = *fmt_list;
  new_list->deque = *deque;
  if (fmt_data != NULL) {
    new_list->fmt_node_data = fmt_data;
  } else {
    new_list->fmt_node_data = *default_fmt_node_data;
  }
  return new_list;
}

int count(LinkedList *list) {
  int size = 0;
  ListNode *node = list->head;
  while (node != NULL) {
    size++;
    node = node->next;
  }
  return size;
}

void drop_list(LinkedList *list) {
  if (list->drop_data == NULL) {
    while (list->head != NULL) {
      list->pop(list);
    }
  } else {
    while (list->head != NULL) {
      list->drop_data(list->pop(list));
    }
  }

  free(list);
}

int cmp(const void *self, const void *other) { return 0; }

int delete_node(LinkedList *self, void *to_be_deleted) {
  printf("delete_node");
  return 0;
}

void push(LinkedList *self, void *to_be_pushed) {
  if (self->head == NULL) {
    self->head = initialize_list_node(to_be_pushed);
    self->tail = self->head;
  } else {
    ListNode *new_node = initialize_list_node(to_be_pushed);
    ListNode *last_node = self->tail;
    last_node->next = new_node;
    new_node->previous = last_node;
    self->tail = new_node;
  }
}

void *pop(LinkedList *self) {
  if (self->tail != NULL) {
    ListNode *last_node = self->tail;

    self->tail = last_node->previous;
    if (self->tail != NULL) {
      self->tail->next = NULL;
    } else {
      self->head = NULL;
    }
    void *data = last_node->data;
    free(last_node);
    return data;
  } else {
    return NULL;
  }
}

void *deque(LinkedList *self) {
  if (self->head != NULL) {
    ListNode* head_node = self->head;
    self->head = head_node->next;
    if (self->head != NULL) {
      self->head->previous = NULL;
    } else {
      self->tail = NULL;
    }
    void *data = head_node->data;
    free(head_node);
    return data;
  } 
  return NULL;
}

ListNode *next_node(ListNode *node) { return node != NULL ? node->next : NULL; }

void drop(LinkedList *self) { drop_list(self); }

void print_list(LinkedList *list, FILE *stream) {
  char *list_formatted = list->fmt(list);
  fprintf(stream, "%s\n", list_formatted);
  free(list_formatted);
}

char* fmt_list(LinkedList* list) {
  ListNode *node = list->head;
  char* str = malloc(sizeof(char)*3);
  str[0] = '(';
  str[1] = '\0';
  int len = 3;
  while (node != NULL) {
    char* node_str = list->fmt_node_data(node->data);
    len += strlen(node_str) + 2;
    str = realloc(str, len);
    strcat(str, node_str);
    free(node_str);
    node = list->next_node(node);
    if (node != NULL) {
      strcat(str, "->");
    }
  }
  strcat(str, ")");
  return str; 
}

void print_node(LinkedList *list, void *to_be_printed, FILE *stream) {
  ListNode *node = (ListNode *)to_be_printed;
  char *data_formatted = list->fmt_node_data(node->data);
  fprintf(stream, "%s", data_formatted);
  free(data_formatted);
}

char *default_fmt_node_data(void *data) {
  char *output = malloc(sizeof(char) * 32);
  sprintf(output, "%p", data);
  return output;
}

void iterate(LinkedList *self, void (*cb)(void *data, int i, void *aux_arg),
             void *aux_arg) {
  int i = 0;
  if (cb != NULL && self != NULL) {
    ListNode *node = self->head;
    while (node != NULL) {
      cb(node->data, i, aux_arg);
      node = node->next;
      i++;
    }
  }
}