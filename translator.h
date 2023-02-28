#pragma once

#include <stdbool.h>
#include <alsa/asoundlib.h>

#include "list.h"
#include "parser.h"

struct event_list {
    struct list l;
    snd_seq_event_t e;
    bool start_loop;
    bool end_loop;
    unsigned int loop_offset;
};

struct event_list *translate(struct node n);

// debug functions
void print_events(struct event_list *l);

