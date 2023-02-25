#include <alsa/asoundlib.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "scheduler.h"


#define DEFAULT_CLIENT_NAME "Korlessa" // TODO: I have two such defaults
#define DEFAULT_QUEUE_SIZE 64
#define DEFAULT_BPM 120 // TODO: make as an argument
#define DEFAULT_PULSE_PER_QUARTER 96 // TODO: I have two such defaults
#define DEFAULT_DRAIN_SIZE 32
#define DEFAULT_DRAIN_CYCLE_SIZE 16
#define DEFAULT_POLL_TIMEOUT 1000 // ms

static volatile sig_atomic_t running = 1;

static void sig_handler(int s)
{
    signal(s, SIG_IGN);
    running = 0;
}

void clear_queue(snd_seq_t *client, int queue_id) {
    snd_seq_remove_events_t *re = NULL;
    int err = snd_seq_remove_events_malloc(&re);
    if (err < 0) {
       fprintf(stderr, "failed allocating remove events structure: %s", snd_strerror(err)); 
       return;
    }
    snd_seq_remove_events_set_queue(re, queue_id);
    snd_seq_remove_events_set_condition(re, SND_SEQ_REMOVE_OUTPUT|SND_SEQ_REMOVE_IGNORE_OFF);
    err = snd_seq_remove_events(client, re);
    if (err < 0) {
        fprintf(stderr, "failed removing events: %s\n", snd_strerror(err));
    }
    snd_seq_remove_events_free(re);
}

int set_tempo(snd_seq_t *client, int queue_id, int bpm) {
    snd_seq_queue_tempo_t *qt = NULL;

    double tempo = 6e7 / (double) bpm;

    int err = snd_seq_queue_tempo_malloc(&qt);
    if (err < 0) {
        fprintf(stderr, "failed allocating queue tempo structure: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    snd_seq_queue_tempo_set_tempo(qt, (unsigned int) tempo);
    snd_seq_queue_tempo_set_ppq(qt, DEFAULT_PULSE_PER_QUARTER);
    err = snd_seq_set_queue_tempo(client, queue_id, qt);
    if (err < 0) {
        fprintf(stderr, "failed changing queue tempo: %s\n", snd_strerror(err));
        snd_seq_queue_tempo_free(qt);
        return EXIT_FAILURE;
    }

    snd_seq_queue_tempo_free(qt);
    return EXIT_SUCCESS;
}

// TODO: come up with a better name
int drain_output(struct event_list *list, snd_seq_t *client, int *index, int n) {

    struct event_list *entry = list_goto(list, *index);
    int i = *index;
    for (int k = 0; entry != NULL; entry = entry->l.next, i++, k++) {
        if (k == n)
            break;
        int err = snd_seq_event_output(client, &entry->e);
        if (err < 0) {
            fprintf(stderr, "failed outputing event: %s\n", snd_strerror(err)); 
            return EXIT_FAILURE;
        }
    }

    *index = i;

    int err = snd_seq_drain_output(client);
    if (err < 0) {
       fprintf(stderr, "failed draining output: %s", snd_strerror(err));
       return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int loop(struct event_list *list, snd_seq_t *client, int *index) {
    int nfds = snd_seq_poll_descriptors_count(client, POLLIN);
    struct pollfd *pfds = calloc(nfds, sizeof(struct pollfd)); 

    if (pfds == NULL)
        return EXIT_FAILURE;

    snd_seq_poll_descriptors(client, pfds, nfds, POLLIN);

    running = 1;
    while (running) {
        int ret = poll(pfds, nfds, DEFAULT_POLL_TIMEOUT);

        if (ret < 0) {
            fprintf(stderr, "poll error occurred: %s", strerror(errno));
            // not going to stop here
        }

        if (ret > 0) {
            unsigned short revents;
            int err = snd_seq_poll_descriptors_revents(client, pfds, nfds, &revents);
            if (err < 0) {
                fprintf(stderr, "failed getting revents: %s", snd_strerror(err));
                goto FAIL_1;
            }

            if (revents > 0) {
                snd_seq_event_t *e;
                snd_seq_event_input(client, &e);
                switch (e->type) {
                case SND_SEQ_EVENT_USR0: // stop the sequencer and program
                    printf("got usr0; stopping poll\n");
                    running = 0;
                    break;

                case SND_SEQ_EVENT_USR1: // drain another output
                    printf("got usr1; draining output\n");
                    if (drain_output(list, client, index, DEFAULT_DRAIN_CYCLE_SIZE) == EXIT_FAILURE)
                        goto FAIL_1;
                    break;

                case SND_SEQ_EVENT_TEMPO: // change tempo
                    printf("got tempo; changing tempo\n");
                    set_tempo(client, e->data.queue.queue, e->data.queue.param.value);
                    break;
                }
                snd_seq_free_event(e);
            }
        }
    }

    free(pfds);
    return EXIT_SUCCESS;

FAIL_1:
    free(pfds);
    return EXIT_FAILURE;
}
 
int run(struct event_list *list, snd_seq_t *client, int queue_id) {

    int err = set_tempo(client, queue_id, DEFAULT_BPM);
    if (err == EXIT_FAILURE)
        return EXIT_FAILURE;

    err = snd_seq_control_queue(client, queue_id, SND_SEQ_EVENT_START, 0, NULL);
    if (err < 0) {
        fprintf(stderr, "failed starting queue: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    int index = 0;

    err = drain_output(list, client, &index, DEFAULT_DRAIN_SIZE);
    if (err == EXIT_FAILURE)
        goto FAIL_1;

    signal(SIGINT, sig_handler); // catch ctrl+c
    err = loop(list, client, &index);
    if (err == EXIT_FAILURE)
        goto FAIL_1;

    clear_queue(client, queue_id);
    snd_seq_control_queue(client, queue_id, SND_SEQ_EVENT_STOP, 0, NULL);
    sleep(1);
    return EXIT_SUCCESS;

FAIL_1:
    clear_queue(client, queue_id);
    snd_seq_control_queue(client, queue_id, SND_SEQ_EVENT_STOP, 0, NULL);
    sleep(1);
    return EXIT_FAILURE;
}

// prepare fills up remaining info for events and adds breakpoints
int prepare(struct event_list *list, int client_id, int port_out, int port_in, int queue_id) {

    int i = 0;
    for (struct event_list *entry = list; entry != NULL; entry = entry->l.next, i++) {
        snd_seq_event_t *e = &entry->e;
        switch (e->type) {
        case SND_SEQ_EVENT_NOTE:
            snd_seq_ev_set_source(e, port_out);
            e->queue = queue_id;
            break;

        case SND_SEQ_EVENT_USR0:
            snd_seq_ev_set_dest(e, client_id, port_in); 
            e->queue = queue_id;
            break;

        case SND_SEQ_EVENT_TEMPO:
            snd_seq_ev_set_dest(e, client_id, port_in); 
            e->queue = queue_id;
            e->data.queue.queue = queue_id;
            break;

        default:
            fprintf(stderr, "unhandled midi event: %u\n", e->type);
            break;
        }

        // TODO: rewrite
        if (((i+1)%(DEFAULT_DRAIN_CYCLE_SIZE-1)) == 0 && i != 0) {
            struct event_list *ev = calloc(1, sizeof(struct event_list));
            if (ev == NULL) return EXIT_FAILURE;
            snd_seq_ev_clear(&ev->e);
            ev->e.type = SND_SEQ_EVENT_USR1;
            snd_seq_ev_set_dest(&ev->e, client_id, port_in);
            snd_seq_ev_schedule_tick(&ev->e, queue_id, 0, e->time.tick);
            ev->l.next = entry->l.next;
            entry->l.next = ev;
            entry = ev; // to move forward in list
        }
    }
    return EXIT_SUCCESS;
}

int schedule_and_loop(struct event_list *list, int target_client, int target_port) {

    snd_seq_t *client;
    int err = snd_seq_open(&client, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (err < 0) {
        fprintf(stderr, "failed opening sequencer: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    int client_id = snd_seq_client_id(client);
    if (client_id < 0) {
        fprintf(stderr, "failed obtaining client id: %s\n", snd_strerror(client_id));
        goto FAIL_1;
    }

    err = snd_seq_set_client_name(client, DEFAULT_CLIENT_NAME);
    if (err < 0) {
        fprintf(stderr, "failed setting client name: %s\n", snd_strerror(err));
        goto FAIL_1;
    }

    int port_out = snd_seq_create_simple_port(client, "groove-out", SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_APPLICATION);
    if (port_out < 0) {
        fprintf(stderr, "failed opening out port: %s\n", snd_strerror(port_out));
        goto FAIL_1;
    }

    int port_in = snd_seq_create_simple_port(client, "groove-in", SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
    if (port_in < 0) {
        fprintf(stderr, "failed opening in port: %s\n", snd_strerror(port_in));
        goto FAIL_2;
    }

    err = snd_seq_connect_to(client, port_out, target_client, target_port);
    if (err < 0) {
        fprintf(stderr, "failed connecting to device: %s\n", snd_strerror(err));
        goto FAIL_3;
    }

    int queue_id = snd_seq_alloc_queue(client);
    if (queue_id < 0) {
        fprintf(stderr, "failed preparing queue: %s\n", snd_strerror(queue_id));
        goto FAIL_4;
    }

    // Valgrind reporting error here!
    err = snd_seq_set_client_pool_output(client, DEFAULT_QUEUE_SIZE);
    if (err < 0) {
        fprintf(stderr, "failed setting pool output: %s\n", snd_strerror(err));
        goto FAIL_5;
    }

    int ret = prepare(list, client_id, port_out, port_in, queue_id);
    if (ret == EXIT_FAILURE)
        goto FAIL_5;

    ret = run(list, client, queue_id);
    if (ret == EXIT_FAILURE)
        goto FAIL_5;

    snd_seq_free_queue(client, queue_id);
    snd_seq_disconnect_to(client, port_out, target_client, target_port);
    snd_seq_delete_simple_port(client, port_in);
    snd_seq_delete_simple_port(client, port_out);
    snd_seq_close(client);
    return EXIT_SUCCESS;

FAIL_5:
    snd_seq_free_queue(client, queue_id);
FAIL_4:
    snd_seq_disconnect_to(client, port_out, target_client, target_port);
FAIL_3:
    snd_seq_delete_simple_port(client, port_in);
FAIL_2:
    snd_seq_delete_simple_port(client, port_out);
FAIL_1:
    snd_seq_close(client);
    return EXIT_FAILURE;
}
