#pragma once

#include <alsa/asoundlib.h>

#include "list.h"
#include "parser.h"

struct event_list {
    struct list l;
    snd_seq_event_t e;
    // struct event_list *next;
};

// void free_event_list(struct event_list *list);

struct event_list *translate(struct node n);

// debug functions
void print_events(struct event_list *l);

