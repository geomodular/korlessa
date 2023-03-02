#pragma once

#include <stdbool.h>

struct list {
    void *next;
};

// list_append inserts the `new_entry` on the end of the list and returns the
// head of the `list`. If `list` is empty it returns `new_entry`.
void *list_append(void *list, void *new_entry);

// list_goto returns the nth element of the `list` or NULL.
void *list_goto(void *list, int index);

// list_goto_last returns the last element of the `list` or NULL if `list` is NULL.
void *list_goto_last(void *list);

// list_insert_after inserts the `new_entry` after the nth element in the `list`.
void list_insert_after(void *list, int index, void *new_entry);

// list_find searches the list and retrieves the element based on `f` function.
// The `f` function should return true if the first argument satisfy the search
// condition. The second argument holds the value passed as `arg` to function. 
void *list_find(void *list, bool (*f)(void *, void *), void *arg);

// list_size traverses over the `list` and counts the elements. Function returns
// the number of elements.
int list_size(void *list);

// list_apply traverses over the `list` and runs function `f` on each element.
// It's safe to use `free()` to destroy the whole list.
void list_apply(void *list, void (*f)(void *));

// list_drop_apply drops the last element in the list and applies `f` on it.
// The expected use is to remove and free the last element. Function returns
// head of the`list`, or NULL if nothing left.
void *list_drop_apply(void *list, void (*f)(void *));

// list_sort performs stupid bubble sort on top of `list`.
void *list_sort(void *list, bool (*f)(void *, void *));

