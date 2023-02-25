#include <assert.h>
#include <alsa/asoundlib.h>

#include "list.h"
#include "parser.h"
#include "translator.h"

#define PULSE_PER_QUARTER 96


struct event_list *new_event_list(snd_seq_event_t e) {
    struct event_list *ptr = calloc(1, sizeof(struct event_list));
    if (ptr == NULL) return NULL;
    ptr->e = e;
    return ptr;
}

struct sheet_reference {
    struct list l;
    struct sheet *s;
    char *label;
    double divider;
};

struct sheet_reference *new_sheet_reference(struct sheet *s, char *label, double divider) {
    struct sheet_reference *ptr = calloc(1, sizeof(struct sheet_reference));
    if (ptr == NULL) return NULL;
    ptr->s = s;
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

struct namespace {
    struct list l;
    const char *label;
};

struct namespace *new_namespace(const char *label) {
    struct namespace *ptr = calloc(1, sizeof(struct namespace));
    if (ptr == NULL) return NULL;
    ptr->label = label;
    return ptr;
}

char *get_label(struct namespace *ns) {

    // Count
    int len = 0;
    int elements = 0;
    for (struct namespace *n = ns; n != NULL; n = n->l.next) {
        len += strlen(n->label);
        elements++;
    }

    if (elements == 0) return NULL;

    // Count dots `.` and `\0` in
    len += elements - 1;
    len++;

    // Allocate
    char *tmp = calloc(len, sizeof(char));
    if (tmp == NULL) return NULL;
    char *full_label = calloc(len, sizeof(char));
    if (full_label == NULL) return NULL;

    // Build the label up
    sprintf(full_label, "%s", ns->label);
    for (struct namespace *n = ns->l.next; n != NULL; n = n->l.next) {
        sprintf(tmp, "%s.%s", full_label, n->label);
        strcpy(full_label, tmp);
    }

    free(tmp);

    return full_label;
}

struct context {
    unsigned int bpm;
    unsigned int offset;
    struct sheet_reference *sheets;
    struct namespace *namespace;
    unsigned char channel;
    double divider;
    unsigned char velocity; 
    int  last_octave;
    struct event_list *last_note_entry; // just one entry is relevant (not list)
    bool referencing; // true if translator is referencing a sheet
};

struct context init_context() {
    return (struct context) {
        .bpm = 120,
        .offset = 0,
        .sheets = NULL,
        .namespace = NULL,
        .channel = 0,
        .divider = 1,
        .velocity = 127,
        .last_octave = 5,
        .last_note_entry = NULL,
        .referencing = false,
    };
}

snd_seq_event_t translate_note(struct context *ctx, struct note *n);
snd_seq_event_t translate_interval(struct context *ctx, snd_seq_event_t event, int interval);
snd_seq_event_t translate_eof(struct context *ctx);
snd_seq_event_t translate_tempo(struct context *ctx, unsigned int tempo);

// Duration of the note in ticks
unsigned int compute_duration(double divider) {
    double d = (PULSE_PER_QUARTER * 4) / divider;
    unsigned int ret = (unsigned int) d;
    if (d - ret > 0)
        printf("warning, not in pulse\n");
    return ret;
}

bool isNotEmpty(char *str) {
    if (str == NULL) return true;
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
        if (note->octave == -1)
            note->octave = ctx->last_octave;
        if (note->channel == -1)
            note->channel = ctx->channel;
        snd_seq_event_t e = translate_note(ctx, note);
        struct event_list *entry = new_event_list(e);
        ctx->last_octave = note->octave;
        ctx->last_note_entry = entry;
        ctx->offset += compute_duration(ctx->divider);
        return entry;
    }

    case NODE_TYPE_INTERVAL:
    {
        if (ctx->last_note_entry != NULL) {
            snd_seq_event_t e = translate_interval(ctx, ctx->last_note_entry->e, n->u.interval->value);
            struct event_list *entry = new_event_list(e);
            ctx->last_note_entry = entry;
            ctx->offset += compute_duration(ctx->divider);
            return entry;
        }
        ctx->offset += compute_duration(ctx->divider);
        break;
    }

    case NODE_TYPE_REST:
        ctx->last_note_entry = NULL;
        ctx->offset += compute_duration(ctx->divider);
        break;

    case NODE_TYPE_TIE:
    {
        unsigned int d = compute_duration(ctx->divider); 
        if (ctx->last_note_entry != NULL)
            ctx->last_note_entry->e.data.note.duration += d;
        ctx->offset += d;
        break;
    }

    case NODE_TYPE_DIVIDER:
        // No action for divider
        break;

    case NODE_TYPE_SHEET:
    {
        if (isNotEmpty(n->u.sheet->label) && !ctx->referencing) {
            ctx->namespace = list_append(ctx->namespace, new_namespace(n->u.sheet->label));
            char *label = get_label(ctx->namespace);
            printf("got: %s\n", label);
            struct sheet_reference *r = new_sheet_reference(n->u.sheet, label, ctx->divider); 
            ctx->sheets = list_append(ctx->sheets, r);
        }
        double d = ctx->divider;
        struct event_list *list = NULL;
        for (size_t i = 0; i < n->u.sheet->repeat_count; i++) {
            for (size_t j = 0; j < n->u.sheet->n; j++) {
                double duration = n->u.sheet->duration;
                int units = n->u.sheet->units;
                ctx->divider = d * (duration / (double) units);
                struct event_list *part = _translate(ctx, n->u.sheet->nodes[j]);
                list = list_append(list, part);
            }
        }
        if (isNotEmpty(n->u.sheet->label) && !ctx->referencing)
            ctx->namespace = list_drop_apply(ctx->namespace, free);
        ctx->divider = d;
        return list;
    }

    case NODE_TYPE_REFERENCE:
    {
        struct sheet_reference *r = list_find(ctx->sheets, find_label_in_sheet_reference, n->u.reference->label);
        if (r != NULL) {
            ctx->referencing = true;
            double d = ctx->divider;
            struct event_list *list = NULL;
            for (size_t i = 0; i < n->u.reference->repeat_count; i++) {
                for (size_t j = 0; j < r->s->n; j++) {
                    ctx->divider = r->divider * (r->s->duration / (double) r->s->units); // reset value on each iteration
                    struct event_list *part = _translate(ctx, r->s->nodes[j]);
                    list = list_append(list, part);
                }
            }
            ctx->referencing = false;
            ctx->divider = d;
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
        };
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
    };

    return NULL;
}

struct event_list *translate(struct node n) {
    struct context ctx = init_context();
    struct event_list *events = _translate(&ctx, &n);
    list_apply(ctx.sheets, free_sheet_reference); 
    return events;
}

int letter_value(char letter) {
    switch(letter) {
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
        } else if (n->accidental[i] == 'b'){
            offset--;
        }
    }
    unsigned int octave = 12 * n->octave;
    return letter_value(n->letter) + offset + octave;
}

snd_seq_event_t translate_note(struct context *ctx, struct note *n) {

    unsigned int tick = compute_duration(ctx->divider);
    tick -= 4; // without this the note would sound like legato

    snd_seq_event_t e;
    snd_seq_ev_clear(&e);
    snd_seq_ev_set_subs(&e);
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    snd_seq_ev_set_note(&e, ctx->channel, midi_value(n), ctx->velocity, tick);
    return e;
}

snd_seq_event_t translate_interval(struct context *ctx, snd_seq_event_t event, int interval) {

    unsigned int tick = compute_duration(ctx->divider);
    tick -= 4; // without this the note would sound like legato
    
    snd_seq_event_t e = event;
    snd_seq_ev_schedule_tick(&e, 0, 0, ctx->offset);
    if (e.data.note.note + interval >= 0) {
        e.data.note.note += interval;
    }
    e.data.note.duration = tick;
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

void print_events(struct event_list *l) {
    if (l == NULL) {
        printf("\n");
        return;
    }

    snd_seq_event_t e = l->e;

    switch (e.type) {
        case SND_SEQ_EVENT_NOTE:
        {
            snd_seq_ev_note_t n = e.data.note;
            printf("(NOTE t:%u ch:%u d:%u n:%u v:%u) ", e.time.tick, n.channel, n.duration, n.note, n.velocity);
            break;
        }

        case SND_SEQ_EVENT_TEMPO:
            printf("(TEMPO t:%u bpm:%d) ", e.time.tick, e.data.queue.param.value);
            break;

        case SND_SEQ_EVENT_USR0:
            printf("(USR0 t:%u) ", e.time.tick);
            break;
    }

    print_events(l->l.next);
}
