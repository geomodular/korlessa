#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <alsa/asoundlib.h>

#include "korlessa.h"
#include "list.h"
#include "parser.h"
#include "translator.h"

struct event_list *new_event_list(snd_seq_event_t e) {
    struct event_list *ptr = calloc(1, sizeof (struct event_list));

    if (ptr == NULL)
        return NULL;
    ptr->e = e;
    return ptr;
}

struct sheet_reference {
    struct list l;
    struct node *node;
    char *label;
    double divider;
};

struct sheet_reference *new_sheet_reference(struct node *node, char *label, double divider) {
    struct sheet_reference *ptr = calloc(1, sizeof (struct sheet_reference));

    if (ptr == NULL)
        return NULL;
    ptr->node = node;
    ptr->label = label;
    ptr->divider = divider;
    return ptr;
}

bool find_label_in_sheet_reference(void *reference, void *arg) {
    struct sheet_reference *r = reference;
    char *label = arg;

    return strcmp(r->label, label) == 0;
}

void free_sheet_reference(void *r) {
    struct sheet_reference *ptr = r;

    free(ptr->label);
    free(ptr);
}

void free_after_loop(void *event, void *ctx) {
    struct event_list *e = event;
    bool *is_loop = ctx;

    if (*is_loop) {
        free(e);
        return;
    }

    if (e->end_loop) {
        e->l.next = NULL;
        *is_loop = true;
    }
}

struct namespace {
    struct list l;
    const char *label;
};

struct namespace *new_namespace(const char *label) {
    struct namespace *ptr = calloc(1, sizeof (struct namespace));

    if (ptr == NULL)
        return NULL;
    ptr->label = label;
    return ptr;
}

char *get_label(struct namespace *ns) {

    // Count
    int len = 0;
    int elements = 0;

    for (struct namespace * n = ns; n != NULL; n = n->l.next) {
        len += strlen(n->label);
        elements++;
    }

    if (elements == 0)
        return NULL;

    // Count dots `.` in
    len += elements - 1;
    // Count `\0` in
    len++;

    // Allocate
    char *tmp = calloc(len, sizeof (char));

    if (tmp == NULL)
        return NULL;

    char *full_label = calloc(len, sizeof (char));

    if (full_label == NULL)
        return NULL;

    // Build the label up
    snprintf(full_label, len, "%s", ns->label);
    for (struct namespace *n = ns->l.next; n != NULL; n = n->l.next) {
        snprintf(tmp, len, "%s.%s", full_label, n->label);
        snprintf(full_label, len, "%s", tmp);
    }

    free(tmp);

    return full_label;
}

struct context {
    unsigned int bpm;
    unsigned int offset;
    int octave;
    struct sheet_reference *sheets;
    struct namespace *namespace;
    unsigned char channel;
    double divider;
    int velocity;
    struct event_list *last_note;
    struct event_list *prev_tone; // Might be note or interval
    bool did_rest;
    bool referencing;
    bool legato;
};

struct context init_context() {

    // Default values
    return (struct context) {
        .bpm = 120,
        .offset = 0,
        .octave = 5,
        .sheets = NULL,
        .namespace = NULL,
        .channel = 0,
        .divider = 1.,
        .velocity = 9,
        .last_note = NULL,
        .prev_tone = NULL,
        .did_rest = false,
        .referencing = false,
        .legato = false,
    };
}

snd_seq_event_t translate_note(struct context *ctx, struct note *n);
snd_seq_event_t translate_interval(struct context *ctx, snd_seq_event_t event, struct interval *i);
snd_seq_event_t translate_controller(struct context *ctx, struct controller *c);
snd_seq_event_t translate_program(struct context *ctx, struct program *p);
snd_seq_event_t translate_eof(struct context *ctx);
snd_seq_event_t translate_tempo(struct context *ctx, unsigned int tempo);
void print_event(struct event_list *l, FILE * f);

// Duration of the note in ticks
unsigned int compute_duration(double divider) {
    double d = (PULSE_PER_QUARTER * 4) / divider;
    unsigned int ret = (unsigned int) d;

    if (d - ret > 0)
        fprintf(stderr, "warning, not in pulse\n");
    return ret;
}

bool isNotEmpty(char *str) {
    if (str == NULL)
        return true;
    return str[0] != '\0';
}

struct event_list *_translate(struct context *ctx, struct node *n) {
    switch (n->type) {

    case NODE_TYPE_BPM:
    {
        snd_seq_event_t e = translate_tempo(ctx, n->u.bpm->value);

        ctx->bpm = n->u.bpm->value;
        return new_event_list(e);
    }

    case NODE_TYPE_NOTE:
    {
        struct note *note = n->u.note;

        if (note->channel == -1)
            note->channel = ctx->channel;

        if (note->octave == -1)
            note->octave = ctx->octave;

        if (note->velocity == -1)
            note->velocity = ctx->velocity;

        snd_seq_event_t e = translate_note(ctx, note);
        struct event_list *entry = new_event_list(e);

        ctx->channel = note->channel;
        ctx->octave = note->octave;
        ctx->velocity = note->velocity;
        ctx->last_note = entry;
        ctx->prev_tone = entry;
        ctx->did_rest = false;
        ctx->offset += compute_duration(ctx->divider);
        return entry;
    }

    case NODE_TYPE_INTERVAL:
    {
        if (ctx->last_note != NULL) {
            snd_seq_event_t e = translate_interval(ctx, ctx->last_note->e, n->u.interval);
            struct event_list *entry = new_event_list(e);

            ctx->prev_tone = entry;
            ctx->did_rest = false;
            ctx->offset += compute_duration(ctx->divider);
            return entry;
        }
        ctx->offset += compute_duration(ctx->divider);
        break;
    }

    case NODE_TYPE_REST:
        ctx->did_rest = true;
        ctx->offset += compute_duration(ctx->divider);
        break;

    case NODE_TYPE_TIE:
    {
        unsigned int d = compute_duration(ctx->divider);

        if (ctx->prev_tone != NULL && !ctx->did_rest)
            ctx->prev_tone->e.data.note.duration += d;
        ctx->offset += d;
        break;
    }

    case NODE_TYPE_CONTROLLER:
    {
        snd_seq_event_t e = translate_controller(ctx, n->u.controller);
        struct event_list *entry = new_event_list(e);

        return entry;
    }

    case NODE_TYPE_PROGRAM:
    {
        snd_seq_event_t e = translate_program(ctx, n->u.program);
        struct event_list *entry = new_event_list(e);

        return entry;
    }

    case NODE_TYPE_DIVIDER:
        // No action for divider
        break;

    case NODE_TYPE_LEGATO:
    {
        ctx->legato = true;
        struct event_list *list = NULL;

        for (size_t i = 0; i < n->n; i++) {
            if (i == (n->n - 1))
                ctx->legato = false; // Legato is turned off on the last element
            struct event_list *part = _translate(ctx, n->nodes[i]);

            list = list_append(list, part);
        }
        return list;
    }

    case NODE_TYPE_SHEET:
    {
        // Namespace and reference handling
        if (isNotEmpty(n->u.sheet->label) && !ctx->referencing) {
            ctx->namespace = list_append(ctx->namespace, new_namespace(n->u.sheet->label));
            char *label = get_label(ctx->namespace);
            struct sheet_reference *r = new_sheet_reference(n, label, ctx->divider);

            ctx->sheets = list_append(ctx->sheets, r);
        }

        // Loop preparations
        bool loop = false;
        bool dry_run = false;
        bool first_to_loop = false;
        int count = n->u.sheet->repeat_count;

        // Loop is indicated by negative number of repeat_count
        if (count < 0) {
            count = 1;
            loop = true;
        } else if (count == 0) {
            count = 1;
            dry_run = true;
        }

        unsigned int old_offset = ctx->offset;
        double d = ctx->divider;

        struct event_list *list = NULL;

        for (size_t i = 0; i < count; i++) {
            for (size_t j = 0; j < n->n; j++) {
                double duration = n->u.sheet->duration;
                int units = n->u.sheet->units;

                ctx->divider = d * (duration / (double) units);
                struct event_list *part = _translate(ctx, n->nodes[j]);

                if (dry_run) {
                    list_apply(part, free);
                    continue;
                };

                // Loop flag for this sheet and first element
                if (loop && !first_to_loop) {
                    if (part != NULL) {
                        part->start_loop = true;
                        first_to_loop = true;
                    }
                }

                // Loop flag for this sheet and last element
                if (loop && j == (n->n - 1)) {
                    if (ctx->prev_tone != NULL) {
                        ctx->prev_tone->end_loop = true;
                        ctx->prev_tone->loop_offset = ctx->offset - old_offset;
                    }
                }

                list = list_append(list, part);
            }
        }

        if (dry_run)
            ctx->offset = old_offset;
        if (isNotEmpty(n->u.sheet->label) && !ctx->referencing)
            ctx->namespace = list_drop_apply(ctx->namespace, free);

        ctx->divider = d;
        return list;
    }

    case NODE_TYPE_REFERENCE:
    {
        struct sheet_reference *r = list_find(ctx->sheets, find_label_in_sheet_reference, n->u.reference->label);

        if (r != NULL) {

            // Loop preparations
            bool loop = false;
            bool dry_run = false;
            bool first_to_loop = false;
            int count = n->u.reference->repeat_count;

            if (count < 0) {
                count = 1;
                loop = true;
            } else if (count == 0) {
                count = 1;
                dry_run = true;
            }

            unsigned int old_offset = ctx->offset;

            ctx->referencing = true;
            double d = ctx->divider;

            struct event_list *list = NULL;

            for (size_t i = 0; i < count; i++) {
                for (size_t j = 0; j < r->node->n; j++) {

                    // Reset value on each iteration
                    ctx->divider = r->divider * (r->node->u.sheet->duration / (double) r->node->u.sheet->units);
                    struct event_list *part = _translate(ctx, r->node->nodes[j]);

                    if (dry_run) {
                        list_apply(part, free);
                        continue;
                    };

                    // Loop flag for this sheet and first element
                    if (loop && !first_to_loop) {
                        if (part != NULL) {
                            part->start_loop = true;
                            first_to_loop = true;
                        }
                    }

                    // Loop flag for this sheet and last element/note
                    if (loop && j == (r->node->n - 1)) {
                        if (ctx->prev_tone != NULL) {
                            ctx->prev_tone->end_loop = true;
                            ctx->prev_tone->loop_offset = ctx->offset - old_offset;
                        }
                    }

                    list = list_append(list, part);
                }
            }

            if (dry_run)
                ctx->offset = old_offset;
            ctx->divider = d;
            ctx->referencing = false;
            return list;
        }
        break;
    }

    case NODE_TYPE_CRATE:
    {
        struct event_list *list = NULL;

        for (size_t i = 0; i < n->n; i++) {
            struct event_list *part = _translate(ctx, n->nodes[i]);

            list = list_append(list, part);
        }
        return list;
    }

    case NODE_TYPE_EOF:
    {
        snd_seq_event_t e = translate_eof(ctx);

        return new_event_list(e);
    }

    case NODE_TYPE_UNKNOWN:
    default:
        break;
    }

    return NULL;
}

bool sort_by_tick(void *e1, void *e2) {
    struct event_list
    *event1 = e1, *event2 = e2;

    return event1->e.time.tick > event2->e.time.tick;
}

struct event_list *translate(struct node n) {
    struct context ctx = init_context();
    struct event_list *events = _translate(&ctx, &n);

    bool is_loop = false;
    list_apply_ctx(events, free_after_loop, &is_loop);
    list_apply(ctx.sheets, free_sheet_reference);
    return events;
}

int letter_value(char letter) {
    switch (letter) {
    case 'C':
    case 'c':
        return 0;
    case 'D':
    case 'd':
        return 2;
    case 'E':
    case 'e':
        return 4;
    case 'F':
    case 'f':
        return 5;
    case 'G':
    case 'g':
        return 7;
    case 'A':
    case 'a':
        return 9;
    case 'B':
    case 'b':
        return 11;
    }
    // unreachable
    assert(0);
    return -1;
}

unsigned char midi_value(struct note *n) {
    int offset = 0;
    size_t len = strlen(n->accidental);

    for (size_t i = 0; i < len; i++) {
        if (n->accidental[i] == '#') {
            offset++;
        } else if (n->accidental[i] == 'b') {
            offset--;
        }
    }
    unsigned int octave = 12 * n->octave;

    return letter_value(n->letter) + offset + octave;
}

snd_seq_event_t translate_note(struct context *ctx, struct note *n) {

    unsigned int tick = compute_duration(ctx->divider);

    if (ctx->legato) {
        tick++;
    } else {
        tick -= 4;
    }

    unsigned char velocity = (double) n->velocity / 9. * 127.;

    snd_seq_event_t e;

    snd_seq_ev_clear(&e);
    snd_seq_ev_set_subs(&e);
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    snd_seq_ev_set_note(&e, n->channel, midi_value(n), velocity, tick);
    return e;
}

snd_seq_event_t translate_interval(struct context *ctx, snd_seq_event_t event, struct interval *i) {

    unsigned int tick = compute_duration(ctx->divider);

    if (ctx->legato) {
        tick++;
    } else {
        tick -= 4;
    }

    snd_seq_event_t e = event;

    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    if (e.data.note.note + i->value >= 0) {
        e.data.note.note += i->value;
    }
    e.data.note.duration = tick;
    return e;
}

snd_seq_event_t translate_controller(struct context *ctx, struct controller *c) {

    snd_seq_event_t e;

    snd_seq_ev_clear(&e);
    snd_seq_ev_set_subs(&e);
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    e.type = SND_SEQ_EVENT_CONTROLLER;
    e.data.control.channel = ctx->channel;
    e.data.control.param = c->param;
    e.data.control.value = c->value;
    return e;
}

snd_seq_event_t translate_program(struct context *ctx, struct program *p) {

    snd_seq_event_t e;

    snd_seq_ev_clear(&e);
    snd_seq_ev_set_subs(&e);
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    e.type = SND_SEQ_EVENT_PGMCHANGE;
    e.data.control.channel = ctx->channel;
    e.data.control.value = p->value;
    return e;
}

snd_seq_event_t translate_eof(struct context *ctx) {

    snd_seq_event_t e;

    snd_seq_ev_clear(&e);
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    e.type = SND_SEQ_EVENT_USR0;
    return e;
}

snd_seq_event_t translate_tempo(struct context *ctx, unsigned int tempo) {

    snd_seq_event_t e;

    snd_seq_ev_clear(&e);
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    e.type = SND_SEQ_EVENT_TEMPO;
    e.data.queue.param.value = tempo; // unsafe conversion?
    return e;
}

void print_event(struct event_list *l, FILE * f) {

    snd_seq_event_t e = l->e;

    switch (e.type) {
    case SND_SEQ_EVENT_NOTE:
    {
        snd_seq_ev_note_t n = e.data.note;
        char *loop = "";

        if (l->start_loop && l->end_loop) {
            loop = "L-START-END ";
        } else if (l->start_loop) {
            loop = "L-START ";
        } else if (l->end_loop) {
            loop = "L-END ";
        }

        fprintf(f, "(NOTE %st:%u ch:%u d:%u n:%u v:%u)", loop, e.time.tick, n.channel, n.duration, n.note, n.velocity);
        break;
    }

    case SND_SEQ_EVENT_CONTROLLER:
        fprintf(f, "(CC p:%u v:%d)", e.data.control.param, e.data.control.value);
        break;

    case SND_SEQ_EVENT_PGMCHANGE:
        fprintf(f, "(PGM v:%d)", e.data.control.value);
        break;

    case SND_SEQ_EVENT_TEMPO:
        fprintf(f, "(TEMPO t:%u bpm:%d)", e.time.tick, e.data.queue.param.value);
        break;

    case SND_SEQ_EVENT_USR0:
        fprintf(f, "(USR0 t:%u)", e.time.tick);
        break;
    }
}

void print_events(struct event_list *l, FILE * f) {
    print_event(l, f);
    if (l->l.next != NULL) {
        fprintf(f, " ");
        print_events(l->l.next, f);
    }
}
