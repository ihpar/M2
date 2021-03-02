
#ifndef EPUCKER_MY_UTILS_H
#define EPUCKER_MY_UTILS_H

#define CODE_LEN 6
#define ALPHABET_LEN 26

extern int alphabet[ALPHABET_LEN][CODE_LEN];

void staller(int dur);

void stall_ms(long dur);

int *create_random_word(int max_w_len, int complexity);

#endif //EPUCKER_MY_UTILS_H
