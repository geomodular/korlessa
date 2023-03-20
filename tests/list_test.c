#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utest.h"
#include "../list.h"

struct person {
    struct list l;
    char *name;
};

typedef struct person Person;


Person *get_static_list();

void test_initialize(struct test *t) {
    Person p = {.name = "Mirka"};
    void *v = list_append(NULL, &p);
    if (v != &p)
        fail(t, "v should initialize to p");
}

void test_append(struct test *t) {
    Person p1 = {.name = "Martin"};
    Person p2 = {.name = "Marek"};
    list_append(&p1, &p2);
    if (p1.l.next != &p2)
        fail(t, "p1 should point to p2");
}

void test_size_of_full_list(struct test *t) {
    Person *ps = get_static_list();
    int size = list_size(ps);
    if (size != 5)
        failf(t, "expected size 5 got %d", size);
}

void test_size_of_one_element(struct test *t) {
    Person p = {.name = "Mojmir"};
    int size = list_size(&p);
    if (size != 1)
        failf(t, "expected size 1 got %d", size);
}

void test_size_of_null(struct test *t) {
    int size = list_size(NULL);
    if (size != 0)
        failf(t, "expected size 0 got %d", size);
}

void test_go_to_first_element(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_goto(ps, 0);
    if (strcmp("Maria", p->name) != 0)
        failf(t, "expected name Maria got %s", p->name);
}

void test_go_to_index_3(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_goto(ps, 3);
    if (strcmp("Martina", p->name) != 0)
        failf(t, "expected name Martina got %s", p->name);
}

void test_go_to_last_element(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_goto_last(ps);
    if (strcmp("Monika", p->name) != 0)
        failf(t, "expected name Monika got %s", p->name);
}

void test_go_to_non_existing_index(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_goto(ps, 20);
    if (p != NULL)
        fail(t, "p should be NULL");
}

void test_go_over_null_list(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_goto(NULL, 0);
    if (p != NULL)
        fail(t, "p should be NULL");
}

void test_go_to_last_element_over_null_list(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_goto_last(NULL);
    if (p != NULL)
        fail(t, "p should be NULL");
}

void test_insert_after_first_element(struct test *t) {
    Person *ps = get_static_list();
    Person new_p = {.name = "Margita"};
    list_insert_after(ps, 0, &new_p);
    Person *p = list_goto(ps, 1);

    int size = list_size(ps);
    if (size != 6)
        failf(t, "expected size 6 got %d", size);

    if (p == NULL) {
        fail(t, "p should not be NULL");
        return;
    }

    if (strcmp("Margita", p->name) != 0)
        failf(t, "expected name Margita got %s", p->name);
}

void test_insert_after_last_element(struct test *t) {
    Person *ps = get_static_list();
    Person new_p = {.name = "Melania"};
    list_insert_after(ps, 4, &new_p);
    Person *p = list_goto(ps, 5);

    int size = list_size(ps);
    if (size != 6)
        failf(t, "expected size 6 got %d", size);

    if (p == NULL) {
        fail(t, "p should not be NULL");
        return;
    }

    if (strcmp("Melania", p->name) != 0)
        failf(t, "expected name Melania got %s", p->name);
}

void test_insert_into_null_list(struct test *t) {
    Person new_p = {.name = "Miska"};
    list_insert_after(NULL, 0, &new_p);
    if (new_p.l.next != NULL)
        fail(t, "next item should stay NULL");
}

bool check_name(void *person, void *name) {
    Person *p = person;
    char *n = name;
    return strcmp(p->name, n) == 0;
}

void test_find(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_find(ps, check_name, "Mima");

    if (p == NULL) {
        fail(t, "p should not be NULL");
        return;
    }

    if (strcmp(p->name, "Mima") != 0)
        failf(t, "expected name Mima got %s", p->name);
}

void test_find_phony(struct test *t) {
    Person *ps = get_static_list();
    Person *p = list_find(ps, check_name, "Matilda");

    if (p != NULL)
        fail(t, "p should be NULL");
}

void change_name(void *person) {
    static char *c = "Name";
    Person *p = person;
    p->name = c;
}

void test_apply(struct test *t) {
    Person *ps = get_static_list();
    list_apply(ps, change_name);

    for (Person *p = ps; p != NULL; p = p->l.next) {
        if (strcmp(p->name, "Name") != 0)
            failf(t, "expected name Name got %s", p->name);
    }
}

void sum_names(void *person, void *ctx) {
    int *sum = ctx;
    *sum += 1;
}

void test_apply_ctx(struct test *t) {
    Person *ps = get_static_list();

    int sum = 0;
    list_apply_ctx(ps, sum_names, &sum);

    if (sum != 5)
        failf(t, "expected sum 5 got %d", sum);
}

void test_drop_apply(struct test *t) {
    Person *ps = get_static_list();
    Person *last = list_goto_last(ps);
    Person *p = list_drop_apply(ps, change_name);
    
    int size = list_size(p);
    if (size != 4)
        failf(t, "expected size 4 got %d", size);

    if (strcmp(last->name, "Name") != 0)
        failf(t, "expected name Name got %s", p->name);
}

void test_drop_apply_null_list(struct test *t) {
    Person *p = list_drop_apply(NULL, change_name);
    if (p != NULL)
        fail(t, "p should be NULL");
}

bool compare_name(void *person1, void *person2) {
    Person *p1 = person1;
    Person *p2 = person2;
    size_t i;

    for (i = 0; p1->name[i] == p2->name[i]; i++) {
        if (p1->name[i] == '\0')
            return true;
    }

    if ((unsigned char) p1->name[i] < (unsigned char) p2->name[i])
        return false;

    return true;
}

void test_sort(struct test *t) {
    char *sorted[] = {"Maria", "Marta", "Martina", "Mima", "Monika"};

    Person *ps = get_static_list();

    size_t i = 0;
    for (Person *p = list_sort(ps, compare_name); p != NULL; p = p->l.next, i++) {
    if (strcmp(p->name, sorted[i]) != 0)
        failf(t, "expected %d. entry Maria got %s", i+1, p->name);
    }
}

int main(int argc, char **argv) {

    static test_fn tests[] = {
        test_initialize,
        test_append,
        test_size_of_full_list, 
        test_size_of_one_element, 
        test_size_of_null,
        test_go_to_first_element,
        test_go_to_index_3,
        test_go_to_last_element,
        test_go_to_non_existing_index,
        test_go_over_null_list,
        test_go_to_last_element_over_null_list, 
        test_insert_after_first_element,
        test_insert_after_last_element,
        test_insert_into_null_list,
        test_find,
        test_find_phony,
        test_apply,
        test_apply_ctx,
        test_drop_apply,
        test_drop_apply_null_list,
        test_sort,
        NULL,
    };

    if (run("Linked list", tests))
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

Person *get_static_list() {
    static Person ps[5];

    ps[0] = (Person) {.l.next = NULL, .name = "Maria"};
    ps[1] = (Person) {.l.next = NULL, .name = "Marta"};
    ps[2] = (Person) {.l.next = NULL, .name = "Mima"};
    ps[3] = (Person) {.l.next = NULL, .name = "Martina"};
    ps[4] = (Person) {.l.next = NULL, .name = "Monika"};

    list_append(ps, &ps[1]);
    list_append(ps, &ps[2]);
    list_append(ps, &ps[3]);
    list_append(ps, &ps[4]);

    return ps;
}

