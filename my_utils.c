#define CODE_LEN 6
#define ALPHABET_LEN 26

void staller(int dur) {
    long i;
    for (i = 0; i < dur * 1000000; i++)
            asm("nop");
}

void stall_ms(long dur) {
    long i;
    for (i = 0; i < dur; i++)
            asm("nop");
}

int alphabet[ALPHABET_LEN][CODE_LEN] = {
        {1, 2,  -1, -1, -1, -1},    // A
        {2, 1,  1,  1,  -1, -1},    // B
        {2, 1,  2,  1,  -1, -1},    // C
        {2, 1,  1,  -1, -1, -1},    // D
        {1, -1, -1, -1, -1, -1},    // E
        {1, 1,  2,  1,  -1, -1},    // F
        {2, 2,  1,  -1, -1, -1},    // G
        {1, 1,  1,  1,  -1, -1},    // H
        {1, 1,  -1, -1, -1, -1},    // I
        {1, 2,  2,  2,  -1, -1},    // J
        {2, 1,  2,  -1, -1, -1},    // K
        {1, 2,  1,  1,  -1, -1},    // L
        {2, 2,  -1, -1, -1, -1},    // M
        {2, 1,  -1, -1, -1, -1},    // N
        {2, 2,  2,  -1, -1, -1},    // O
        {1, 2,  2,  1,  -1, -1},    // P
        {2, 2,  1,  2,  -1, -1},    // Q
        {1, 2,  1,  -1, -1, -1},    // R
        {1, 1,  1,  -1, -1, -1},    // S
        {2, -1, -1, -1, -1, -1},    // T
        {1, 1,  2,  -1, -1, -1},    // U
        {1, 1,  1,  2,  -1, -1},    // V
        {1, 2,  2,  -1, -1, -1},    // W
        {2, 1,  1,  2,  -1, -1},    // X
        {2, 1,  2,  2,  -1, -1},    // Y
        {2, 2,  1,  1,  -1, -1}     // Z
};

void create_random_word(int *word, int max_w_len, int complexity) {
    int i;
    int current_bit = -1,
            last_bit = -1;

    for (i = 0; i < complexity; i++) {
        word[i] = 1;
    }
}

