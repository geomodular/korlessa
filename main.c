#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "listing.h"
#include "parser.h"
#include "scheduler.h"
#include "translator.h"

#define OPT_DEBUG 1
#define OPT_PRINT_AST 2
#define OPT_PRINT_EVENTS 3
#define OPT_CLIENT 4
#define OPT_PORT 5

const char *argp_program_version = "v0.1";
const char *argp_program_bug_address = "<geomodular@gmail.com>";
static char doc[] = "Korlessa is a notation to a midi events translator.";

static struct argp_option options[] = {
    {"debug", OPT_DEBUG, 0, 0, "Print additional debug info."},
    {"print-ast", OPT_PRINT_AST, 0, 0, "Print ast produced by parser and quit."},
    {"print-events", OPT_PRINT_EVENTS, 0, 0, "Print translated midi events and quit."},
    {"list", 'l', 0, 0, "List clients and ports to connect to."},
    {"file", 'f', "FILENAME", 0, "Use file as an input instead of stdin."},
    {"source", 's', "CODE", 0, "Use this input instead of stdin."},
    {"connect-to", 'c', "ADDRESS", 0, "Device address to connect to in a <client>:<port> format."},
    {"client", OPT_CLIENT, "CLIENT_ID", 0, "Client id to connect to."},
    {"port", OPT_PORT, "PORT_ID", 0, "Port of the client to connect to."},
    {0}
};

struct arguments {
    bool debug;
    bool print_ast;
    bool print_events;
    bool list_clients;
    char *filepath;
    char *source;
    char *address;
    int client;
    int port;
};

struct arguments init_arguments() {
    return (struct arguments) {
        .debug = false,
        .print_ast = false,
        .print_events = false,
        .list_clients = false,
        .filepath = NULL,
        .source = NULL,
        .address = NULL,
        .client = 0,
        .port = 0,
    };
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
    case OPT_DEBUG:
        arguments->debug = true;
        break;
    case OPT_PRINT_AST:
        arguments->print_ast = true;
        break;
    case OPT_PRINT_EVENTS:
        arguments->print_events = true;
        break;
    case 'l':
        arguments->list_clients = true;
        break;
    case 'f':
        arguments->filepath = arg;
        break;
    case 's':
        arguments->source = arg;
        break;
    case OPT_CLIENT:
        arguments->client = atoi(arg);
        break;
    case OPT_PORT:
        arguments->port = atoi(arg);
        break;
    case 'c':
    {
        char *token = NULL;
        int client = strtol(arg, &token, 10);

        if (token[0] != ':')
            argp_error(state, "invalid address format: %s should be <client>:<port>", arg);
        int port = strtol(&token[1], NULL, 10);

        arguments->client = client;
        arguments->port = port;
        arguments->address = arg;
        break;
    }
    case ARGP_KEY_ARG:
        return 0;
    case ARGP_KEY_END:
        if (arguments->client == 0 &&
            arguments->print_ast == false && arguments->print_events == false && arguments->list_clients == false)
            argp_failure(state, EXIT_FAILURE, 0, "use -c to connect to device");
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc, 0, 0, 0 };

char *read_stdin();

int main(int argc, char **argv) {

    struct arguments args = init_arguments();

    argp_parse(&argp, argc, argv, 0, 0, &args);

    if (args.list_clients) {
        return list_devices();
    }

    struct parser p = new_parser();
    struct parse_result res;

    if (args.source) {
        res = parse("<arg>", p, args.source);
    } else if (args.filepath) {
        FILE *f = fopen(args.filepath, "r");

        if (f == NULL) {
            fprintf(stderr, "failed opening file: %s\n", args.filepath);
            goto FAIL_1;
        }
        res = parse_file(args.filepath, p, f);
        fclose(f);
    } else {
        char *in = read_stdin();

        if (in == NULL) {
            fprintf(stderr, "failed reading from stdin\n");
            goto FAIL_1;
        }
        res = parse("<stdin>", p, in);
        free(in);
    };

    if (res.err != NULL) {
        fprintf(stderr, "%s", res.err);
        goto FAIL_2;
    }

    if (args.print_ast) {
        print_ast(res.n, stdout);
        printf("\n");
        goto SUCCESS_2;
    }

    struct event_list *list = translate(*res.n);

    if (args.print_events) {
        print_events(list, stdout);
        printf("\n");
        goto SUCCESS_3;
    }
    // Up to this point there should be no memory leaks
    if (schedule_and_loop(list, args.client, args.port) == EXIT_FAILURE) {
        goto FAIL_3;
    }

SUCCESS_3:
    list_apply(list, free);
SUCCESS_2:
    free_parse_result(&res);
    free_parser(&p);
    return EXIT_SUCCESS;

FAIL_3:
    list_apply(list, free);
FAIL_2:
    free_parse_result(&res);
FAIL_1:
    free_parser(&p);
    return EXIT_FAILURE;
}

char *read_stdin() {
    size_t buffer_size = 1024, i = 0;
    char *buffer = calloc(sizeof (char), buffer_size + 1);

    if (buffer == NULL)
        return NULL;

    for (int c = getchar(); c != EOF; c = getchar()) {
        if (i == buffer_size) {
            buffer_size += 1024;
            void *ptr = realloc(buffer, buffer_size * sizeof (char) + 1);

            if (ptr != NULL) {
                buffer = (char *) ptr;
            } else {
                free(buffer);
                return NULL;
            }
        }
        buffer[i++] = c; // possible data lost?
    }

    buffer[i] = '\0';
    return buffer;
}
