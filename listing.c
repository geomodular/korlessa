#include "listing.h"

#include <stdio.h>
#include <stdlib.h>
 
#include <alsa/asoundlib.h>

#define get_bit(i, x) (x & (1 << i))
#define clear_bit(i, x) (x & (~(1 << i)))

#define DEFAULT_CLIENT_NAME "korlessa-listing"

const char* get_capability(unsigned int state) {
    switch (state) {
        case SND_SEQ_PORT_CAP_READ: return "READ";
        case SND_SEQ_PORT_CAP_WRITE: return "WRITE";
        case SND_SEQ_PORT_CAP_SYNC_READ: return "SYNC_READ";
        case SND_SEQ_PORT_CAP_SYNC_WRITE: return "SYNC_WRITE";
        case SND_SEQ_PORT_CAP_DUPLEX: return "DUPLEX";
        case SND_SEQ_PORT_CAP_SUBS_READ: return "SUBS_READ";
        case SND_SEQ_PORT_CAP_SUBS_WRITE: return "SUBS_WRITE";
        case SND_SEQ_PORT_CAP_NO_EXPORT: return "SUBS_NO_EXPORT";
        default: return "UNKNOWN";
    }
}

const char* get_type(unsigned int state) {
    switch (state) {
        case SND_SEQ_PORT_TYPE_SPECIFIC: return "SPECIFIC";
        case SND_SEQ_PORT_TYPE_MIDI_GENERIC: return "MIDI_GENERIC";
        case SND_SEQ_PORT_TYPE_MIDI_GM: return "MIDI_GM";
        case SND_SEQ_PORT_TYPE_MIDI_GS: return "MIDI_GS";
        case SND_SEQ_PORT_TYPE_MIDI_XG: return "MIDI_XG";
        case SND_SEQ_PORT_TYPE_MIDI_MT32: return "MIDI_MT32";
        case SND_SEQ_PORT_TYPE_MIDI_GM2: return "MIDI_GM2";
        case SND_SEQ_PORT_TYPE_SYNTH: return "SYNTH";
        case SND_SEQ_PORT_TYPE_DIRECT_SAMPLE: return "DIRECT_SAMPLE";
        case SND_SEQ_PORT_TYPE_SAMPLE: return "SAMPLE";
        case SND_SEQ_PORT_TYPE_HARDWARE: return "HARDWARE";
        case SND_SEQ_PORT_TYPE_SOFTWARE: return "SOFTWARE";
        case SND_SEQ_PORT_TYPE_SYNTHESIZER: return "SYNTHESIZER";
        case SND_SEQ_PORT_TYPE_PORT: return "PORT";
        case SND_SEQ_PORT_TYPE_APPLICATION: return "APPLICATION";
        default: return "UNKNOWN";
    }
}

// Iterator function
unsigned int get_next_bit(unsigned int *state) {
    unsigned int _state = *state;
    if (_state != 0) {
        for (int i = 0;; i++) {
            unsigned int bit = get_bit(i, _state);
            if (bit > 0) {
                *state = clear_bit(i, _state);
                return bit;
            }
        }
    }
    return 0; 
}
 
int list_devices() {
    
    snd_seq_t *client;
    int err = snd_seq_open(&client, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (err < 0) {
        fprintf(stderr, "error opening sequencer: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    err = snd_seq_set_client_name(client, DEFAULT_CLIENT_NAME);
    if (err < 0) {
        fprintf(stderr, "error setting client name: %s\n", snd_strerror(err));
        goto FAIL_1;
    }

    snd_seq_client_info_t *cinfo = NULL;
    err = snd_seq_client_info_malloc(&cinfo);
    if (err < 0) {
        fprintf(stderr, "error allocating client info struct: %s\n", snd_strerror(err));
        goto FAIL_1;
    }
    
    snd_seq_port_info_t *pinfo = NULL;
    err = snd_seq_port_info_malloc(&pinfo);
    if (err < 0) {
        fprintf(stderr, "error allocating port info struct: %s\n", snd_strerror(err));
        goto FAIL_2;
    }
    
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(client, cinfo) >= 0) {
        const char *client_name = snd_seq_client_info_get_name(cinfo);
        if (strcmp(client_name, DEFAULT_CLIENT_NAME) == 0) continue;
        int client_id = snd_seq_client_info_get_client(cinfo);
        
        printf("Client: %s\n", client_name);
        
        snd_seq_port_info_set_client(pinfo, client_id);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(client, pinfo) >= 0) {
            const char *port_name = snd_seq_port_info_get_name(pinfo);
            int port = snd_seq_port_info_get_port(pinfo);
            printf("  %d:%d %s", client_id, port, port_name);
            
            unsigned int bit;
            
            printf("\n    capabilities: ");
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            while ((bit = get_next_bit(&caps)) > 0) printf("%s ", get_capability(bit));
            
            unsigned int types = snd_seq_port_info_get_type(pinfo);
            if (types != 0) {
                printf("\n    types: ");
                while ((bit = get_next_bit(&types)) > 0) printf("%s ", get_type(bit));
            }
            
            int channels = snd_seq_port_info_get_midi_channels(pinfo);
            if (channels == 0) {
                printf("\n    no channels");
            } else {
                printf("\n    channels: %d", channels);
            }
            printf("\n");
        }
    }
    
    snd_seq_port_info_free(pinfo);
    snd_seq_client_info_free(cinfo);
    snd_seq_close(client);
    return EXIT_SUCCESS;

FAIL_2:
    snd_seq_client_info_free(cinfo);
FAIL_1:
    snd_seq_close(client);
    return EXIT_FAILURE;
}

