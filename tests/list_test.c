#include <stdio.h>
#include <stdlib.h>
#include "utest.h"
#include "../list.h"

struct person {
    struct list l;
    char *name;
};

typedef struct person Person;

int main(int argc, char **argv) {
    init("Linked list");

    utest("Append", {
        Person p1 = {.name = "Peter"};
        Person p2 = {.name = "Marek"};
        list_append(&p1, &p2);
        equal(p1.l.next == &p2, "p1 should point to p2");
    });

    utest("Initialize", {
        Person p1 = {.name = "Mirka"};
        void *v = list_append(NULL, &p1);
        equal(v == &p1, "v should initialize to p1");
    });

    Person ps[] = {
        {.l.next = NULL, .name = "Maria"},
        {.l.next = NULL, .name = "Marta"},
        {.l.next = NULL, .name = "Mima"},
        {.l.next = NULL, .name = "Martina"},
        {.l.next = NULL, .name = "Monika"},
    };

    list_append(ps, &ps[1]);
    list_append(ps, &ps[2]);
    list_append(ps, &ps[3]);
    list_append(ps, &ps[4]);

    utest("Size of full list", {
        int size = list_size(ps);
        equal(size == 5, "size != 5");
    });

    utest("Size of one element", {
        int size = list_size(&ps[4]);
        equal(size == 1, "size != 1");
    });

    utest("Size of NULL", {
        int size = list_size(NULL);
        equal(size == 0, "size != 0");
    });

    print_results();
    return EXIT_SUCCESS;
}

