#include <assert.h>
#include <stdlib.h>
#include "lib/mpc.h"
#include "parser.h"


mpc_val_t *bpm_fold(int n, mpc_val_t ** xs);
mpc_val_t *note_fold(int n, mpc_val_t ** xs);
mpc_val_t *interval_fold(int n, mpc_val_t ** xs);
mpc_val_t *sheet_fold(int n, mpc_val_t ** xs);
mpc_val_t *node_fold(int n, mpc_val_t ** xs);
mpc_val_t *reference_fold(int n, mpc_val_t ** xs);
mpc_val_t *duration_fold(int n, mpc_val_t ** xs);
mpc_val_t *repeater_fold(int n, mpc_val_t ** xs);
mpc_val_t *controller_fold(int n, mpc_val_t ** xs);
mpc_val_t *program_fold(int n, mpc_val_t ** xs);
mpc_val_t *ctor_int_default();
mpc_val_t *ctor_one();
mpc_val_t *apply_rest(mpc_val_t * x);
mpc_val_t *apply_tie(mpc_val_t * x);
mpc_val_t *apply_divider(mpc_val_t * x);
mpc_val_t *apply_eof(mpc_val_t * x);
mpc_val_t *apply_as_duration(mpc_val_t * x);
mpc_val_t *apply_loop(mpc_val_t * x);
mpc_val_t *apply_legato(mpc_val_t * x);
void free_node(mpc_val_t * x);

struct parser new_parser() {

    mpc_parser_t *note = mpc_new("note");
    mpc_parser_t *rest = mpc_new("rest");
    mpc_parser_t *tie = mpc_new("tie");
    mpc_parser_t *divider = mpc_new("divider");
    mpc_parser_t *controller = mpc_new("controller");
    mpc_parser_t *program = mpc_new("program");
    mpc_parser_t *repeater = mpc_new("repeater");
    mpc_parser_t *comment = mpc_new("comment");
    mpc_parser_t *reference = mpc_new("reference");
    mpc_parser_t *interval = mpc_new("interval");
    mpc_parser_t *label = mpc_new("label");
    mpc_parser_t *sheet = mpc_new("sheet");
    mpc_parser_t *legato = mpc_new("legato");
    mpc_parser_t *parser = mpc_new("parser");

    // Note: ch2:c#4
    mpc_parser_t *channel = mpc_and(3, mpcf_snd_free, mpc_string("ch"), mpc_int(), mpc_char(':'), free, free);
    mpc_parser_t *letter = mpc_oneof("cdefgabCDEFGAB");
    mpc_parser_t *accidental = mpc_many1(mpcf_strfold, mpc_oneof("#b"));
    mpc_parser_t *octave = mpc_int();
    mpc_parser_t *velocity = mpc_apply(mpc_and(2, mpcf_snd_free, mpc_char('!'), mpc_digit(), free), mpcf_int);

    mpc_define(note, mpc_and(5, note_fold,
                             mpc_maybe_lift(channel, ctor_int_default),
                             letter,
                             mpc_maybe_lift(accidental, mpcf_ctor_str),
                             mpc_maybe_lift(octave, ctor_int_default),
                             mpc_maybe_lift(velocity, ctor_int_default), free, free, free, free));

    // Bpm: 120bpm
    mpc_parser_t *bpm = mpc_and(2, bpm_fold,
                                mpc_digits(),
                                mpc_string("bpm"),
                                free);

    // Rest: .
    mpc_define(rest, mpc_apply(mpc_char('.'), apply_rest));

    // Tie: -
    mpc_define(tie, mpc_apply(mpc_char('-'), apply_tie));

    // Divider: |
    mpc_define(divider, mpc_apply(mpc_char('|'), apply_divider));

    // Control change: cc9:129
    mpc_define(controller, mpc_and(4, controller_fold,
                                   mpc_or(2, mpc_string("cc"), mpc_string("CC")),
                                   mpc_digits(), mpc_char(':'), mpc_int(), free, free, free));

    // Program change: pgm12
    mpc_define(program, mpc_and(2, program_fold, mpc_or(2, mpc_string("pgm"), mpc_string("PGM")), mpc_digits(), free));

    // Comment: // comment
    mpc_define(comment, mpc_and(2, mpcf_all_free, mpc_string("//"), mpc_many1(mpcf_strfold, mpc_noneof("\n")), free));

    // Reference: {label} {label1.label2}
    mpc_define(reference, mpc_and(2, reference_fold,
                                  mpc_tok_brackets(mpc_and(2, mpcf_strfold,
                                                           mpc_ident(),
                                                           mpc_many(mpcf_strfold,
                                                                    mpc_and(2, mpcf_strfold, mpc_char('.'), mpc_ident(),
                                                                            free)), free), free), repeater, free));

    // Intervals: +2 -2
    mpc_define(interval, mpc_and(2, interval_fold, mpc_oneof("-+"), mpc_digits(), free));

    // Legato: (a +1 c)
    mpc_define(legato,
               mpc_apply(mpc_tok_parens
                         (mpc_many1(node_fold, mpc_or(3, mpc_tok(note), mpc_tok(interval), mpc_tok(divider))),
                          free_node), apply_legato));

    // Label (part of sheet and group): myLabel:
    mpc_define(label, mpc_and(2, mpcf_fst_free, mpc_ident(), mpc_char(':'), free));

    mpc_parser_t *duration = mpc_or(4,
                                    mpc_and(3, duration_fold, mpc_digits(), mpc_string("is"), mpc_digits(), free, free),
                                    mpc_and(3, duration_fold, mpc_digits(), mpc_string("as"), mpc_digits(), free, free),
                                    mpc_and(3, duration_fold, mpc_digits(), mpc_string("to"), mpc_digits(), free, free),
                                    mpc_apply(mpc_digits(), apply_as_duration)
      );

    // Repeater (part of reference and sheet): {}x2
    mpc_define(repeater, mpc_maybe_lift(mpc_or(2,
                                               mpc_and(2, repeater_fold,
                                                       mpc_char('x'),
                                                       mpc_digits(),
                                                       free), mpc_apply(mpc_string("loop"), apply_loop)), ctor_one));

    // Sheet: label:4to3{...}
    mpc_define(sheet, mpc_and(4, sheet_fold,
                              mpc_maybe_lift(label, mpcf_ctor_str),
                              duration,
                              mpc_tok_brackets(mpc_many(node_fold, mpc_or(10,
                                                                          mpc_tok(rest),
                                                                          mpc_tok(interval),
                                                                          mpc_tok(tie),
                                                                          mpc_tok(divider),
                                                                          mpc_tok(comment),
                                                                          mpc_tok(legato),
                                                                          mpc_tok(sheet),
                                                                          mpc_tok(controller),
                                                                          mpc_tok(program), mpc_tok(note)
                                                        )), free_node), repeater, free, free, free_node));

    // Top level statements
    mpc_parser_t *crate = mpc_total(mpc_many(node_fold, mpc_or(6,
                                                               mpc_tok(sheet),
                                                               mpc_tok(reference),
                                                               mpc_tok(bpm),
                                                               mpc_tok(controller),
                                                               mpc_tok(program),
                                                               mpc_tok(comment)
                                             )), free_node);

    // Crate and EOF
    mpc_define(parser, mpc_apply(crate, apply_eof));

    return (struct parser) {
        .note = note,
        .interval = interval,
        .rest = rest,
        .tie = tie,
        .divider = divider,
        .controller = controller,
        .program = program,
        .repeater = repeater,
        .comment = comment,
        .reference = reference,
        .label = label,
        .sheet = sheet,
        .legato = legato,
        .root = parser,
    };
}

void free_parser(struct parser *p) {

    if (p == NULL)
        return;

    mpc_cleanup(14,
                p->note,
                p->interval,
                p->rest,
                p->tie,
                p->divider,
                p->controller,
                p->program, p->repeater, p->comment, p->reference, p->label, p->sheet, p->legato, p->root);

    *p = (struct parser) { 0 };
}

mpc_val_t *bpm_fold(int n, mpc_val_t ** xs) {

    struct node *node = calloc(1, sizeof (struct node));
    struct bpm *bpm = calloc(1, sizeof (struct bpm));

    bpm->value = (unsigned int) strtoul(xs[0], NULL, 10);

    node->type = NODE_TYPE_BPM;
    node->u.bpm = bpm;

    free(xs[0]);
    free(xs[1]);

    return node;
}

mpc_val_t *note_fold(int n, mpc_val_t ** xs) {

    struct node *node = calloc(1, sizeof (struct node));
    struct note *note = calloc(1, sizeof (struct note));

    note->channel = *(int *) xs[0];
    note->letter = *(char *) xs[1];
    note->accidental = xs[2];
    note->octave = *(int *) xs[3];
    note->velocity = *(int *) xs[4];

    node->type = NODE_TYPE_NOTE;
    node->u.note = note;

    free(xs[0]);                // channel/chars
    free(xs[1]);                // letter/char
    // free(xs[2]); // accidental/chars, let's keep this one
    free(xs[3]);                // octave/int
    free(xs[4]);                // velocity/int

    return node;
}

mpc_val_t *interval_fold(int n, mpc_val_t ** xs) {

    struct node *node = calloc(1, sizeof (struct node));
    struct interval *interval = calloc(1, sizeof (struct interval));

    char *sign = xs[0];
    int value = strtoul(xs[1], NULL, 10);

    if (sign[0] == '-')
        value *= -1;

    interval->value = value;

    node->type = NODE_TYPE_INTERVAL;
    node->u.interval = interval;

    free(xs[0]);                // sign/char 
    free(xs[1]);                // digits/chars

    return node;
}

mpc_val_t *sheet_fold(int n, mpc_val_t ** xs) {

    struct sheet *sheet = calloc(1, sizeof (struct sheet));
    struct node *crate = xs[2]; // Let's just reuse the crate for a sheet

    char *ptr = NULL;

    sheet->label = xs[0];
    sheet->units = strtol(xs[1], &ptr, 10);
    sheet->duration = strtol(&ptr[1], NULL, 10);
    sheet->repeat_count = *(int *) xs[3];

    crate->type = NODE_TYPE_SHEET;
    crate->u.sheet = sheet;

    // free(xs[0]); // label/ident
    free(xs[1]);                // duration/digits
    // free(xs[2]); // crate/node
    free(xs[3]);                // repeater/int

    return crate;
}

mpc_val_t *node_fold(int n, mpc_val_t ** xs) {

    int m = 0;

    for (size_t i = 0; i < n; i++)
        if (xs[i] != NULL)
            m++;

    struct node *node = calloc(1, sizeof (struct node));
    struct node **nodes = calloc(m, sizeof (struct node *));

    for (size_t i = 0, j = 0; i < n; i++) {
        if (xs[i] != NULL) {
            nodes[j] = xs[i];
            j++;
        }
    }

    node->type = NODE_TYPE_CRATE;
    node->n = m;
    node->nodes = nodes;

    return node;
}

mpc_val_t *reference_fold(int n, mpc_val_t ** xs) {

    struct node *node = calloc(1, sizeof (struct node));
    struct reference *reference = calloc(1, sizeof (struct reference));

    int *count = xs[1];

    reference->label = xs[0];
    reference->repeat_count = *count;

    node->type = NODE_TYPE_REFERENCE;
    node->u.reference = reference;

    // free(xs[0]); // label/string
    free(xs[1]);                // repeater/digits

    return node;
}

mpc_val_t *duration_fold(int n, mpc_val_t ** xs) {

    char *units = xs[0];
    char *duration = xs[2];
    char *ret = calloc(strlen(units) + strlen(duration) + 2, sizeof (char));

    sprintf(ret, "%s:%s", units, duration);

    free(xs[0]);                // units/string
    free(xs[1]);                // 'is' | 'as' | 'to' /string
    free(xs[2]);                // duration/string

    return ret;
}


mpc_val_t *repeater_fold(int n, mpc_val_t ** xs) {

    int *ret = calloc(1, sizeof (int));

    *ret = atoi(xs[1]);

    free(xs[0]);                // 'x'/char
    free(xs[1]);                // count/int

    return ret;
}

mpc_val_t *controller_fold(int n, mpc_val_t ** xs) {

    struct node *node = calloc(1, sizeof (struct node));
    struct controller *controller = calloc(1, sizeof (struct controller));

    int *value = xs[3];

    controller->param = strtoul(xs[1], NULL, 10);
    controller->value = *value;

    node->type = NODE_TYPE_CONTROLLER;
    node->u.controller = controller;

    free(xs[0]);                // 'cc'/string
    free(xs[1]);                // param/digits
    free(xs[2]);                // ':'/char
    free(xs[3]);                // value/int

    return node;
}

mpc_val_t *program_fold(int n, mpc_val_t ** xs) {

    struct node *node = calloc(1, sizeof (struct node));
    struct program *program = calloc(1, sizeof (struct program));

    program->value = strtoul(xs[1], NULL, 10);  // Conversion to unsigned

    node->type = NODE_TYPE_PROGRAM;
    node->u.program = program;

    free(xs[0]);                // 'pgm'/string
    free(xs[1]);                // value/digits

    return node;
}

mpc_val_t *ctor_int_default() {
    int *x = calloc(1, sizeof (int));

    *x = -1;
    return x;
}

mpc_val_t *ctor_one() {
    int *x = calloc(1, sizeof (int));

    *x = 1;
    return x;
}

mpc_val_t *apply_rest(mpc_val_t * x) {
    struct node *node = calloc(1, sizeof (struct node));

    node->type = NODE_TYPE_REST;
    free(x);
    return node;
}

mpc_val_t *apply_tie(mpc_val_t * x) {
    struct node *node = calloc(1, sizeof (struct node));

    node->type = NODE_TYPE_TIE;
    free(x);
    return node;
}

mpc_val_t *apply_divider(mpc_val_t * x) {
    struct node *node = calloc(1, sizeof (struct node));

    node->type = NODE_TYPE_DIVIDER;
    free(x);
    return node;
}

mpc_val_t *apply_eof(mpc_val_t * x) {
    struct node *n = x;

    assert(n->type == NODE_TYPE_CRATE);

    struct node *node = calloc(1, sizeof (struct node));

    node->type = NODE_TYPE_EOF;

    n->nodes = realloc(n->nodes, (n->n + 1) * sizeof (struct node *));
    n->nodes[n->n] = node;
    n->n += 1;

    return x;
}

mpc_val_t *apply_as_duration(mpc_val_t * x) {
    char *str = x;
    char *ret = calloc(strlen(str) + 3, sizeof (char));

    sprintf(ret, "1:%s", str);
    free(x);
    return ret;
}

mpc_val_t *apply_loop(mpc_val_t * x) {
    int *ret = calloc(1, sizeof (int));

    *ret = -1;
    free(x);
    return ret;
}

mpc_val_t *apply_legato(mpc_val_t * x) {
    struct node *n = x;

    // Let's just keep the node and rename it to legato
    n->type = NODE_TYPE_LEGATO;

    return n;
}

void free_node(mpc_val_t * x) {
    if (x == NULL)
        return;

    struct node *n = x;

    switch (n->type) {
    case NODE_TYPE_BPM:
        free(n->u.bpm);
        n->u.bpm = NULL;
        break;

    case NODE_TYPE_NOTE:
        free(n->u.note->accidental);
        n->u.note->accidental = NULL;
        free(n->u.note);
        n->u.note = NULL;
        break;

    case NODE_TYPE_INTERVAL:
        free(n->u.interval);
        n->u.interval = NULL;
        break;

    case NODE_TYPE_REST:
    case NODE_TYPE_TIE:
    case NODE_TYPE_DIVIDER:
    case NODE_TYPE_EOF:
        break;

    case NODE_TYPE_SHEET:
        for (size_t i = 0; i < n->n; i++) {
            free_node(n->nodes[i]);
            n->nodes[i] = NULL;
        }
        free(n->u.sheet->label);
        n->u.sheet->label = NULL;
        free(n->u.sheet);
        n->u.sheet = NULL;
        free(n->nodes);
        n->nodes = NULL;
        break;

    case NODE_TYPE_REFERENCE:
        free(n->u.reference->label);
        n->u.reference->label = NULL;
        free(n->u.reference);
        n->u.reference = NULL;
        break;

    case NODE_TYPE_CONTROLLER:
        free(n->u.controller);
        n->u.controller = NULL;
        break;

    case NODE_TYPE_PROGRAM:
        free(n->u.program);
        n->u.program = NULL;
        break;

    case NODE_TYPE_LEGATO:
    case NODE_TYPE_CRATE:
        for (size_t i = 0; i < n->n; i++) {
            free_node(n->nodes[i]);
            n->nodes[i] = NULL;
        }
        free(n->nodes);
        n->nodes = NULL;
        break;

    case NODE_TYPE_UNKNOWN:
        break;
    }

    free(n);
}

void print_ast(struct node *n, FILE * f) {
    switch (n->type) {
    case NODE_TYPE_BPM:
        fprintf(f, "(BPM v:%d)", n->u.bpm->value);
        break;

    case NODE_TYPE_NOTE:
        fprintf(f, "(NOTE ch:%d n:%c a:%s o:%d v:%d)", n->u.note->channel, n->u.note->letter, n->u.note->accidental,
                n->u.note->octave, n->u.note->velocity);
        break;

    case NODE_TYPE_INTERVAL:
        fprintf(f, "(INTERVAL v:%d)", n->u.interval->value);
        break;

    case NODE_TYPE_REST:
        fprintf(f, "(REST)");
        break;

    case NODE_TYPE_TIE:
        fprintf(f, "(TIE)");
        break;

    case NODE_TYPE_DIVIDER:
        fprintf(f, "(DIVIDER)");
        break;

    case NODE_TYPE_SHEET:
        fprintf(f, "(SHEET l:%s u:%d d:%d r:%d", n->u.sheet->label, n->u.sheet->units, n->u.sheet->duration,
                n->u.sheet->repeat_count);
        for (size_t i = 0; i < n->n; i++) {
            fprintf(f, " ");
            print_ast(n->nodes[i], f);
        }
        fprintf(f, ")");
        break;

    case NODE_TYPE_LEGATO:
        fprintf(f, "(LEGATO");
        for (size_t i = 0; i < n->n; i++) {
            fprintf(f, " ");
            print_ast(n->nodes[i], f);
        }
        fprintf(f, ")");
        break;

    case NODE_TYPE_REFERENCE:
        fprintf(f, "(REFERENCE l:%s r:%d)", n->u.reference->label, n->u.reference->repeat_count);
        break;

    case NODE_TYPE_CONTROLLER:
        fprintf(f, "(CC p:%u v:%d)", n->u.controller->param, n->u.controller->value);
        break;

    case NODE_TYPE_PROGRAM:
        fprintf(f, "(PGM v:%d)", n->u.program->value);
        break;

    case NODE_TYPE_CRATE:
        fprintf(f, "(CRATE");
        for (size_t i = 0; i < n->n; i++) {
            fprintf(f, " ");
            print_ast(n->nodes[i], f);
        }
        fprintf(f, ")");
        break;

    case NODE_TYPE_EOF:
        fprintf(f, "(EOF)");
        break;

    case NODE_TYPE_UNKNOWN:
        fprintf(f, "(?)");
        break;
    }
}

struct parse_result parse(const char *filename, struct parser p, const char *in) {
    assert(in != NULL);

    mpc_result_t r;

    if (mpc_parse(filename, in, p.root, &r) == 0) {
        char *err = mpc_err_string(r.error);

        mpc_err_delete(r.error);
        return (struct parse_result) {
            .n = NULL,
            .err = err,
        };
    }

    return (struct parse_result) {
        .n = r.output,
        .err = NULL
    };
}

struct parse_result parse_file(const char *filename, struct parser p, FILE * in) {
    assert(in != NULL);

    mpc_result_t r;

    if (mpc_parse_file(filename, in, p.root, &r) == 0) {
        char *err = mpc_err_string(r.error);

        mpc_err_delete(r.error);
        return (struct parse_result) {
            .n = NULL,
            .err = err,
        };
    }

    return (struct parse_result) {
        .n = r.output,
        .err = NULL
    };
}

void free_parse_result(struct parse_result *r) {
    if (r == NULL)
        return;
    if (r->n != NULL) {
        free_node(r->n);
        r->n = NULL;
    }
    if (r->err != NULL) {
        free(r->err);
        r->err = NULL;
    }
}
