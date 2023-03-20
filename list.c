#include <stdbool.h>
#include <stdlib.h>
#include "list.h"


void *list_append(void *list, void *new_entry) {
    if (list == NULL)
        return new_entry;
    struct list *l = list;

    for (; l->next != NULL; l = l->next) ;
    l->next = new_entry;
    return list;
}

void *list_goto(void *list, int index) {
    struct list *l = list;

    for (int i = 0; l != NULL && i < index; l = l->next, i++) ;
    return l;
}

void *list_goto_last(void *list) {
    struct list *l = list;

    if (l == NULL)
        return NULL;
    for (; l->next != NULL; l = l->next) ;
    return l;
}

void list_insert_after(void *list, int index, void *new_entry) {
    struct list *l = list_goto(list, index);

    if (l == NULL)
        return;
    struct list *e = new_entry;

    e->next = l->next;
    l->next = e;
}

void *list_find(void *list, bool (*f)(void *, void *), void *arg) {
    struct list *l = list;

    while (l != NULL) {
        struct list *next = l->next;

        if (f(l, arg))
            return l;
        l = next;
    };
    return NULL;
}

int list_size(void *list) {
    int i = 0;

    for (struct list * l = list; l != NULL; l = l->next, i++) ;
    return i;
}

void list_apply(void *list, void (*f)(void *)) {
    struct list *l = list;

    while (l != NULL) {
        struct list *next = l->next;

        f(l);
        l = next;
    };
}

void list_apply_ctx(void *list, void (*f)(void *, void *), void *ctx) {
    struct list *l = list;

    while (l != NULL) {
        struct list *next = l->next;

        f(l, ctx);
        l = next;
    };
}

void *list_drop_apply(void *list, void (*f)(void *)) {
    struct list *l = list;
    struct list *last = NULL;

    if (l == NULL)
        return NULL;
    for (; l->next != NULL; last = l, l = l->next) ;
    if (f != NULL)
        f(l);
    if (last != NULL) {
        last->next = NULL;
        return list;
    }
    return NULL;
}

struct list *swap(struct list *l1, struct list *l2) {
    void *tmp = l2->next;

    l2->next = l1;
    l1->next = tmp;
    return l2;
}

void *list_sort(void *list, bool (*f)(void *, void *)) {

    int size = list_size(list);
    struct list anchor = {.next = list };

    for (size_t i = 0; i < size; i++) {

        struct list *head = &anchor;
        bool swapped = false;

        for (size_t j = 0; j < size - i - 1; j++) {

            struct list *actual = head->next;
            struct list *next = actual->next;

            if (f(actual, next)) {
                head->next = swap(actual, next);
                head = next;
                swapped = true;
            } else {
                head = actual;
            }
        }
        if (!swapped)
            break;
    }
    return anchor.next;
}
