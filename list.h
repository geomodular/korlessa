#pragma once

#include <stdbool.h>

struct list {
    void *next;
};

void *list_append(void *list, void *new_entry);
void *list_goto(void *list, int index);
void *list_goto_last(void *list);
void list_insert_after(void *list, int index, void *new_entry);
void *list_find(void *list, bool (*f)(void *, void *), void *arg);

// list_size traverses over the list and counts the elements. Function returns
// the number of elements.
int list_size(void *list);

// list_apply traverses over the list and runs function `f` on each element.
// It's safe to use `free()` to destroy the whole list.
void list_apply(void *list, void (*f)(void *));

// list_drop_apply drops the last element in the list and applies `f` on it.
// The expected use is to remove and free the last element. Function returns
// updated list, or NULL if nothing left.
void *list_drop_apply(void *list, void (*f)(void *));

