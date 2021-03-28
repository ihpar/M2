#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "p30f6014A.h"
#include "epuck_ports.h"
#include "uart/e_uart_char.h"
#include "motor_led/e_init_port.h"
#include "codec/e_sound.h"
#include "a_d/advance_ad_scan/e_ad_conv.h"
#include "a_d/advance_ad_scan/e_micro.h"
#include "bluetooth/e_bluetooth.h"

#include "my_utils.h"
#include "talk.h"
#include "listen.h"

#define SIM_THRESHOLD 80
#define MAX_WORD_LEN 12
#define INITIAL_WORD_LEN 8
#define MEMORY_SIZE 10

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int word_node_count = 0;

struct word_node {
    char word[MAX_WORD_LEN];
    int count;
};

struct word_node memory[MEMORY_SIZE];

int get_len(const char *arr, int limit) {
    int len;
    for (len = 0; len < limit; len++) {
        if (arr[len] == '0') {
            break;
        }
    }
    return len;
}

int levenshtein(char *s, int ls, char *t, int lt) {
    int a, b, c;

    if (!ls) return lt;
    if (!lt) return ls;

    if (s[ls - 1] == t[lt - 1])
        return levenshtein(s, ls - 1, t, lt - 1);

    a = levenshtein(s, ls - 1, t, lt - 1);
    b = levenshtein(s, ls, t, lt - 1);
    c = levenshtein(s, ls - 1, t, lt);

    if (a > b) a = b;
    if (a > c) a = c;

    return a + 1;
}

int find_similarity(char *w1, char *w2) {
    int w1_len = get_len(w1, MAX_WORD_LEN);
    int w2_len = get_len(w2, MAX_WORD_LEN);
    int distance = levenshtein(w1, w1_len, w2, w2_len);

    float similarity_percentage = 100 * ((float) (w1_len - distance) / (float) w1_len);
    return (int) similarity_percentage;
}

void insert_word_node_to_queue(struct word_node wn) {
    int i, j, idx, sim_score, max_sim_score;

    max_sim_score = 0;
    for (i = 0; i < word_node_count; i++) {
        // find most similar word in memory
        sim_score = find_similarity(memory[i].word, wn.word);
        if (sim_score > max_sim_score) {
            max_sim_score = sim_score;
            idx = i;
        }
    }

    if (max_sim_score >= SIM_THRESHOLD) {
        // if the most similar word is acceptable, add it to queue again
        for (i = 0; i < MAX_WORD_LEN; i++) {
            wn.word[i] = memory[idx].word[i];
        }
    }

    // add the new word to the end of the queue
    if (word_node_count == MEMORY_SIZE) {
        // shift the queue
        for (j = 0; j < MEMORY_SIZE - 1; j++) {
            for (i = 0; i < MAX_WORD_LEN; i++) {
                memory[j].word[i] = memory[j + 1].word[i];
            }
        }
        // append new word to the end
        for (i = 0; i < MAX_WORD_LEN; i++) {
            memory[word_node_count - 1].word[i] = wn.word[i];
        }
    } else {
        // append to the end of the queue
        for (i = 0; i < MAX_WORD_LEN; i++) {
            memory[word_node_count].word[i] = wn.word[i];
        }
        word_node_count++;
    }
}

void create_random_word_non_grouped(char *word, int word_len, int total_len) {
    int i;
    char bits[2] = {'1', '2'};
    for (i = 0; i < word_len; i++) {
        word[i] = bits[(rand() % 2)];
    }
    for (i = word_len; i < total_len; i++) {
        word[i] = '0';
    }
}

void choose_random_word_from_queue(char *result) {
    int i, chosen_index;
    chosen_index = rand() % word_node_count;
    for (i = 0; i < MAX_WORD_LEN; i++) {
        result[i] = memory[chosen_index].word[i];
    }
}

void send_everything(char *message, char *word, int action) {
    int i, j, k;
    j = 0;

    for (i = 0; i < word_node_count; i++) {
        message[j] = 'M';
        j++;
        message[j] = ':';
        j++;
        for (k = 0; k < MAX_WORD_LEN; k++) {
            message[j] = memory[i].word[k];
            j++;
        }
        message[j] = '\n';
        j++;
    }
    if (action == 1) {
        // listen
        message[j] = 'L';
    } else {
        // speak
        message[j] = 'S';
    }
    j++;
    message[j] = 'W';
    j++;
    message[j] = ':';
    j++;

    if (word[0] != '0') {
        for (i = 0; i < MAX_WORD_LEN; i++) {
            message[j] = word[i];
            j++;
        }
    }

    message[j] = 'X';
    j++;
    message[j] = '\0';

    e_send_uart1_char(message, strlen(message));
    while (e_uart1_sending());
}

int main(void) {
    e_init_port();
    e_init_uart1();
    e_init_sound();
    e_init_ad_scan(MICRO_ONLY);
    e_ad_scan_on();

    staller(4);

    char message[100];
    char command[100];
    int i, ci, rand_seed, r_pos;
    char car, cf;

    char heard_word[MAX_WORD_LEN];
    char random_chosen_word[MAX_WORD_LEN];
    char everything_buffer[11 * (MAX_WORD_LEN + 4)];
    int random_seeded = 0;

    struct word_node wn;

    int char_pos = 0;
    while (1) {
        while (!e_ischar_uart1());
        e_getchar_uart1(&car);

        if (car == 'T') {
            cf = command[0];
            // check command from PC
            if (cf == 'i') {
                sprintf(message, "%s X", "ok-i");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
            } else if (cf == 's') {
                // command is to speak
                // choose a random word from memory
                choose_random_word_from_queue(random_chosen_word);
                // speak the chosen word
                talk(random_chosen_word, MAX_WORD_LEN);
                // send log to PC
                send_everything(everything_buffer, random_chosen_word, 2);
            } else if (cf == 'l') {
                // command is to listen
                for (i = 0; i < MAX_WORD_LEN; i++) {
                    heard_word[i] = '0';
                }
                // listen for the spoken word
                listen(heard_word, MAX_WORD_LEN);

                if (heard_word[0] != '0') {
                    for (i = 0; i < MAX_WORD_LEN; i++) {
                        wn.word[i] = heard_word[i];
                    }
                    wn.count = 1;
                    insert_word_node_to_queue(wn);
                }
                send_everything(everything_buffer, heard_word, 1);
            } else if (cf == 'r') {

                rand_seed = 0;
                for (r_pos = char_pos - 1; r_pos > 0; r_pos--) {
                    ci = command[r_pos] - '0';
                    rand_seed += rand_seed * 10 + ci;
                }
                if (!random_seeded) {
                    srand(rand_seed);
                    create_random_word_non_grouped(wn.word, INITIAL_WORD_LEN, MAX_WORD_LEN);
                    wn.count = 1;
                    insert_word_node_to_queue(wn);
                    random_seeded = 1;
                }
                sprintf(message, "seed: %dX", rand_seed);
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
            } else {
                sprintf(message, "%s X", "nothing");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
            }

            char_pos = 0;
        } else {
            command[char_pos] = car;
            char_pos++;
        }

    }

}

#pragma clang diagnostic pop
