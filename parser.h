#pragma once

#include <stdlib.h>
#include "lib/mpc.h"

struct bpm {
    unsigned int value;
};

struct note {
    int channel;
    char letter;
    char *accidental;
    int octave;
};

struct interval {
    int value;
};

struct sheet {
    char *label;
    int units;
    int duration;
    int repeat_count;
    size_t n;
    struct node **nodes;
};

struct reference {
    char *label;
    int repeat_count;
};

struct controller {
    unsigned int param;
    int value;
};

struct program {
    int value;
};

enum node_type {
    NODE_TYPE_UNKNOWN = 0,
    NODE_TYPE_BPM = 1,
    NODE_TYPE_NOTE = 2,
    NODE_TYPE_INTERVAL = 3,
    NODE_TYPE_REST = 4,
    NODE_TYPE_TIE = 5,
    NODE_TYPE_DIVIDER = 6,
    NODE_TYPE_REWIND = 7,
    NODE_TYPE_CONTROLLER = 8,
    NODE_TYPE_PROGRAM = 9,
    NODE_TYPE_SHEET = 10,
    NODE_TYPE_REFERENCE = 11,
    NODE_TYPE_CRATE = 12,
    NODE_TYPE_EOF = 13,
};

struct node {
    enum node_type type;

    union {
        struct bpm *bpm;
        struct note *note;
        struct interval *interval;
        struct sheet *sheet;
        struct divider *divider;
        struct reference *reference;
        struct controller *controller;
        struct program *program;
    } u;

    // crate
    size_t n;
    struct node **nodes;
};

struct parser {
    mpc_parser_t *note;
    mpc_parser_t *interval;
    mpc_parser_t *rest;
    mpc_parser_t *tie;
    mpc_parser_t *divider;
    mpc_parser_t *rewind;
    mpc_parser_t *controller;
    mpc_parser_t *program;
    mpc_parser_t *repeater;
    mpc_parser_t *comment;
    mpc_parser_t *reference;
    mpc_parser_t *label;
    mpc_parser_t *sheet;
    mpc_parser_t *root;
};

struct parse_result {
    struct node *n;
    char *err;
};

struct parser new_parser();
void free_parser(struct parser *p);

struct parse_result parse(const char *filename, struct parser p, const char *in);
struct parse_result parse_file(const char *filename, struct parser p, FILE *in);
void free_parse_result(struct parse_result *p);

// debug functions
void print_ast(struct node *n);

