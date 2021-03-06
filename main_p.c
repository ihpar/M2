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
#define EPOCH_COUNT 61

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int word_node_count = 0;

struct word_node {
    char word[MAX_WORD_LEN];
    int count;
};

struct word_node memory[EPOCH_COUNT * 2];

int probability_arr[EPOCH_COUNT * 3];

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

int insert_word_node(struct word_node wn) {
    int i, idx, sim_score, max_sim_score;

    max_sim_score = 0;
    for (i = 0; i < word_node_count; i++) {
        sim_score = find_similarity(memory[i].word, wn.word);
        if (sim_score > max_sim_score) {
            max_sim_score = sim_score;
            idx = i;
        }
    }

    if (max_sim_score >= SIM_THRESHOLD) {
        // increment most similar word
        memory[idx].count++;
        return 0; // no new words added
    } else {
        // add new word
        for (i = 0; i < MAX_WORD_LEN; i++) {
            memory[word_node_count].word[i] = wn.word[i];
        }
        memory[word_node_count].count = 1;
        word_node_count++;
        return 1; // 1 new word added
    }
}

void create_random_word(char *word, int max_syl_len, int complexity) {
    int i, j, syl_len;
    char bits[2] = {'1', '2'};

    int idx = 0;
    int offset = rand() % 2;

    for (i = 0; i < complexity; i++) {
        syl_len = 1 + rand() % max_syl_len;

        for (j = 0; j < syl_len; j++) {
            word[idx] = bits[(offset + i) % 2];
            idx++;
        }
    }

    for (i = idx; i < MAX_WORD_LEN; i++) {
        word[i] = '0';
    }
}

void choose_random_word(char *result) {
    int total_weights = 0;
    int i, j, k;
    int chosen_index;

    for (i = 0; i < word_node_count; i++) {
        total_weights += memory[i].count;
    }

    k = 0;
    for (i = 0; i < word_node_count; i++) {
        for (j = 0; j < memory[i].count; j++) {
            probability_arr[k] = i;
            k++;
        }
    }

    chosen_index = probability_arr[rand() % total_weights];

    for (i = 0; i < MAX_WORD_LEN; i++) {
        result[i] = memory[chosen_index].word[i];
    }
}

void send_words_memory_contents(char *word_str, char *line) {
    int i;

    for (i = 0; i < word_node_count; i++) {
        sprintf(word_str, "%s", memory[i].word);
        word_str[MAX_WORD_LEN] = '\0';
        sprintf(line, "M:%s-%d\n", word_str, memory[i].count);

        e_send_uart1_char(line, strlen(line));
        while (e_uart1_sending());
        stall_ms(10);
    }
}

void send_word_message(char *word_str, char *line, char *word, int action) {
    sprintf(word_str, "%s", word);
    word_str[MAX_WORD_LEN] = '\0';

    if (action == 1) {
        sprintf(line, "LW:%sX\n", word_str);
    }
    if (action == 2) {
        sprintf(line, "SW:%sX\n", word_str);
    }

    e_send_uart1_char(line, strlen(line));
    while (e_uart1_sending());
}

int main(void) {
    e_init_port();
    e_init_uart1();
    staller(4);

    char message[50];
    char command[10];
    int i, c, rand_seed;
    int def_comm_count = 1;
    char *c_dummy;

    int max_syl_len = 3;
    int complexity = 4;
    int num_words_in_memory = 0;

    char heard_word[MAX_WORD_LEN];
    char random_chosen_word[MAX_WORD_LEN];

    struct word_node wn;

    char word_str[MAX_WORD_LEN + 2];
    char line[MAX_WORD_LEN + 9];

    while (1) {
        // get command from PC bluetooth
        i = 0, c = 0;
        do {
            if (e_getchar_uart1(&command[i])) {
                c = command[i];
                i++;
            }
        } while (((char) c != '\n') && ((char) c != '\x0d'));
        command[i] = '\0';

        // process command
        switch (command[0]) {
            case 'i':
                // stay idle
                // notify PC that I'm done
                sprintf(message, "ok-i X\n");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
                break;
            case 'l': // listen
                // init "hearing" buffer
                for (i = 0; i < MAX_WORD_LEN; i++) {
                    heard_word[i] = '0';
                }
                // listen for the spoken word
                listen(heard_word, MAX_WORD_LEN);
                if (heard_word[0] == '0') {
                    choose_random_word(heard_word);
                }
                // insert heard word to memory
                for (i = 0; i < MAX_WORD_LEN; i++) {
                    wn.word[i] = heard_word[i];
                }
                wn.count = 1;
                num_words_in_memory += insert_word_node(wn);
                // send log to PC
                send_words_memory_contents(word_str, line);
                send_word_message(word_str, line, heard_word, 1);
                break;
            case 's': // speak
                // choose a random word from memory
                choose_random_word(random_chosen_word);
                // speak the chosen word
                talk(random_chosen_word, MAX_WORD_LEN);
                // send log to PC
                send_words_memory_contents(word_str, line);
                send_word_message(word_str, line, random_chosen_word, 2);
                break;
            default:
                // default: echo command
                // notify PC that I'm done
                if (def_comm_count == 2) {
                    // got a random num from pc
                    rand_seed = (int) strtol(command, &c_dummy, 10);
                    if (rand_seed < 0) {
                        rand_seed = -1 * rand_seed;
                    }
                    srand(rand_seed);
                    create_random_word(wn.word, max_syl_len, complexity);
                    wn.count = 1;
                    num_words_in_memory += insert_word_node(wn);
                }
                def_comm_count++;
                strcat(command, "X\n");
                e_send_uart1_char(command, strlen(command));
                while (e_uart1_sending());
                break;
        }

    }

}

#pragma clang diagnostic pop
