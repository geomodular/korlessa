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

    if (strcmp("Melania", p->name) != 0)
        failf(t, "expected name Melania got %s", p->name);
}

void test_insert_into_null_list(struct test *t) {
    Person new_p = {.name = "Miska"};
    list_insert_after(NULL, 0, &new_p);
    if (new_p.l.next != NULL)
        fail(t, "next item should stay NULL");
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

