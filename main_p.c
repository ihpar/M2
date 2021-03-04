#include <stdio.h>
#include <time.h>
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma clang diagnostic ignored "-Wmissing-noreturn"


int get_len(const int *arr, int limit) {
    int len;
    for (len = 0; len < limit; len++) {
        if (arr[len] == 0) {
            break;
        }
    }
    return len;
}

int levenshtein(int *s, int ls, int *t, int lt) {
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

int find_similarity(int *w1, int *w2, int max_word_len) {
    int w1_len = get_len(w1, max_word_len);
    int w2_len = get_len(w2, max_word_len);
    int distance = levenshtein(w1, w1_len, w2, w2_len);

    float similarity_percentage = 100 * ((float) (w1_len - distance) / (float) w1_len);
    return (int) similarity_percentage;
}

struct word_node {
    int *word;
    int count;
};

struct word_ll {
    struct word_node elem;
    struct word_ll *next;
};

struct word_ll *word_head = NULL;

int insert_word_node(struct word_node wn, int max_word_len) {
    struct word_ll *t, *temp;
    int i, idx, sim_score, max_sim_score;

    t = (struct word_ll *) malloc(sizeof(struct word_ll));

    if (word_head == NULL) {
        // memory is empty, add new word to beginning
        word_head = t;
        word_head->elem.word = (int *) malloc(sizeof(int) * max_word_len);
        for (i = 0; i < max_word_len; i++) {
            word_head->elem.word[i] = wn.word[i];
        }
        word_head->elem.count = wn.count;
        word_head->next = NULL;
        return 1; // 1 new word inserted
    }

    temp = word_head;
    i = 0;
    max_sim_score = 0;
    while (temp != NULL) {
        // find most similar word in memory
        sim_score = find_similarity(temp->elem.word, wn.word, max_word_len);
        if (sim_score > max_sim_score) {
            max_sim_score = sim_score;
            idx = i;
        }
        i++;
        temp = temp->next;
    }

    temp = word_head;
    if (max_sim_score >= SIM_THRESHOLD) {
        // increment most similar word
        i = 0;
        while (temp != NULL) {
            if (i == idx) {
                temp->elem.count++;
                break;
            }
            i++;
            temp = temp->next;
        }
        return 0; // no new words added
    } else {
        // add new word
        while (temp->next != NULL)
            temp = temp->next;

        t->elem.word = (int *) malloc(sizeof(int) * max_word_len);
        for (i = 0; i < max_word_len; i++) {
            t->elem.word[i] = wn.word[i];
        }
        t->elem.count = wn.count;
        temp->next = t;
        t->next = NULL;
        return 1; // 1 new word added
    }
}

void delete_word_ll() {
    struct word_ll *temp;

    while (word_head != NULL) {
        temp = word_head;
        word_head = word_head->next;

        free(temp);
    }
}

void print_word_ll(int max_word_len) {
    int i;
    struct word_ll *temp = word_head;
    while (temp != NULL) {
        for (i = 0; i < max_word_len; i++) {
            printf("%d ", temp->elem.word[i]);
        }
        printf("count: %d\n", temp->elem.count);
        temp = temp->next;
    }
}

void get_words_memory_contents(char *log, int max_word_len) {
    int i;
    struct word_ll *temp = word_head;
    char *word_str = (char *) malloc((max_word_len) * sizeof(char));
    char *line = (char *) malloc((max_word_len + 7) * sizeof(char));

    while (temp != NULL) {
        for (i = 0; i < max_word_len; i++) {
            sprintf(&word_str[i], "%d", temp->elem.word[i]);
        }
        sprintf(line, "M:%s-%d\n", word_str, temp->elem.count);
        strcat(log, line);
        temp = temp->next;
    }
}

void create_random_word(int *word, int max_syl_len, int complexity) {
    int i, j, syl_len;
    int bits[2] = {1, 2};

    int idx = 0;
    int offset = rand() % 2;

    for (i = 0; i < complexity; i++) {
        syl_len = 1 + rand() % max_syl_len;

        for (j = 0; j < syl_len; j++) {
            word[idx] = bits[(offset + i) % 2];
            idx++;
        }
    }
}

void random_w_choose(int *result, int max_word_len) {
    struct word_ll *temp;
    int total_weights = 0;
    int *weight_list;
    int i, j = 0, el_idx = 0;
    int chosen_index;

    temp = word_head;
    while (temp != NULL) {
        total_weights += temp->elem.count;
        temp = temp->next;
    }

    weight_list = (int *) calloc(total_weights, sizeof(int));
    temp = word_head;
    while (temp != NULL) {
        for (i = 0; i < temp->elem.count; i++) {
            weight_list[j] = el_idx;
            j++;
        }
        temp = temp->next;
        el_idx++;
    }

    chosen_index = weight_list[rand() % total_weights];
    temp = word_head;
    i = 0;
    while (temp != NULL) {
        if (i == chosen_index) {
            for (j = 0; j < max_word_len; j++) {
                result[j] = temp->elem.word[j];
            }
            break;
        }
        temp = temp->next;
        i++;
    }
}

void prepare_word_message(char *message, int *word, int max_word_len, int action) {
    char *word_str = (char *) malloc((max_word_len + 1) * sizeof(char));
    int i;
    for (i = 0; i < max_word_len; i++) {
        sprintf(&word_str[i], "%d", word[i]);
    }
    word_str[max_word_len] = '\0';
    if (action == 1) {
        strcpy(message, "LW:");
    }
    if (action == 2) {
        strcpy(message, "SW:");
    }
    strcat(message, word_str);
    strcat(message, "X\n");
}

int main(void) {
    e_init_port();
    e_init_uart1();
    staller(4);

    char message[50];
    char command[10];
    int i, c, rand_seed;
    int def_comm_count = 1;
    char *c_dummy, *mem_dump;

    int max_syl_len = 3;
    int complexity = 4;
    int max_word_len = max_syl_len * complexity;
    int num_words_in_memory = 0;

    int *heard_word = (int *) calloc(max_word_len, sizeof(int));
    int *random_chosen_word = (int *) calloc(max_word_len, sizeof(int));

    struct word_node wn;

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
                sprintf(message, "ok-iX\n");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
                break;
            case 'l': // listen
                // init "hearing" buffer
                for (i = 0; i < max_word_len; i++) {
                    heard_word[i] = 0;
                }

                // listen for the spoken word
                listen(heard_word, max_word_len);
                for (i = 0; i < max_word_len; i++) {
                    wn.word[i] = heard_word[i];
                }
                wn.count = 1;
                // insert heard word to memory
                num_words_in_memory += insert_word_node(wn, max_word_len);

                // notify PC that I'm done
                mem_dump = (char *) calloc((num_words_in_memory + 1) * (max_word_len + 7), sizeof(char));
                get_words_memory_contents(mem_dump, max_word_len);
                prepare_word_message(message, heard_word, max_word_len, 1);
                strcat(mem_dump, message);

                e_send_uart1_char(mem_dump, strlen(mem_dump));
                while (e_uart1_sending());
                free(mem_dump);
                break;
            case 's': // speak
                // choose a random word from memory
                random_w_choose(random_chosen_word, max_word_len);
                // speak the chosen word
                talk(random_chosen_word, max_word_len);

                // notify PC that I'm done
                mem_dump = (char *) calloc((num_words_in_memory + 1) * (max_word_len + 7), sizeof(char));
                get_words_memory_contents(mem_dump, max_word_len);
                prepare_word_message(message, random_chosen_word, max_word_len, 2);
                strcat(mem_dump, message);

                e_send_uart1_char(mem_dump, strlen(mem_dump));
                while (e_uart1_sending());
                free(mem_dump);
                break;
            default:
                // default: echo the command back
                // notify PC that I'm done
                if (def_comm_count == 2) {
                    // got a random num from pc
                    rand_seed = (int) strtol(command, &c_dummy, 10);
                    if (rand_seed < 0) {
                        rand_seed = -1 * rand_seed;
                    }
                    srand(rand_seed);
                    wn.word = (int *) calloc(max_word_len, sizeof(int));
                    create_random_word(wn.word, max_syl_len, complexity);
                    wn.count = 1;
                    num_words_in_memory += insert_word_node(wn, max_word_len);
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
