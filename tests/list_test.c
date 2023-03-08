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

int main(int argc, char **argv) {
    init("Linked list");

    utest("Initialize", {
        Person p1 = {.name = "Mirka"};
        void *v = list_append(NULL, &p1);
        equal(v == &p1, "v should initialize to p1");
    });

    utest("Append", {
        Person p1 = {.name = "Martin"};
        Person p2 = {.name = "Marek"};
        list_append(&p1, &p2);
        equal(p1.l.next == &p2, "p1 should point to p2");
    });

    utest("Size of full list", {
        Person *ps = get_static_list();
        int size = list_size(ps);
        equal(size == 5, "size != 5");
    });

    utest("Size of one element", {
        Person p = {.name = "Mojmir"};
        int size = list_size(&p);
        equal(size == 1, "size != 1");
    });

    utest("Size of NULL", {
        int size = list_size(NULL);
        equal(size == 0, "size != 0");
    });

    utest("Go to first element", {
        Person *ps = get_static_list();
        Person *p = list_goto(ps, 0);
        equal(strcmp("Maria", p->name) == 0, "name != Maria");
    });

    utest("Go to index 3", {
        Person *ps = get_static_list();
        Person *p = list_goto(ps, 3);
        equal(strcmp("Martina", p->name) == 0, "name != Martina");
    });

    utest("Go to last element", {
        Person *ps = get_static_list();
        Person *p = list_goto_last(ps);
        equal(strcmp("Monika", p->name) == 0, "name != Monika");
    });

    utest("Go to non existing index", {
        Person *ps = get_static_list();
        Person *p = list_goto(ps, 20);
        equal(p == NULL, "p != NULL");
    });

    utest("Go to over NULL list", {
        Person *ps = get_static_list();
        Person *p = list_goto(NULL, 0);
        equal(p == NULL, "p != NULL");
    });

    utest("Go to last element over NULL list", {
        Person *ps = get_static_list();
        Person *p = list_goto_last(NULL);
        equal(p == NULL, "p != NULL");
    });

    utest("Insert after first element", {
        Person *ps = get_static_list();
        Person new_p = {.name = "Margita"};
        list_insert_after(ps, 0, &new_p);
        Person *p = list_goto(ps, 1);
        equal(list_size(ps) == 6, "size != 6");
        equal(strcmp(p->name, "Margita") == 0, "name != Margita");
    });

    utest("Insert after last element", {
        Person *ps = get_static_list();
        Person new_p = {.name = "Melania"};
        list_insert_after(ps, 4, &new_p);
        Person *p = list_goto(ps, 5);
        equal(list_size(ps) == 6, "size != 6");
        equal(strcmp(p->name, "Melania") == 0, "name != Melania");
    });

    utest("Insert into NULL list", {
        Person new_p = {.name = "Melania"};
        list_insert_after(NULL, 0, &new_p);
        equal(new_p.l.next == NULL, "next item should stay the same")
    });

    print_results();
    if (_error > 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
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

