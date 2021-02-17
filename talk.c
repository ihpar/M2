#include "codec/e_sound.h"
#include "my_utils.h"
#include "a_d/advance_ad_scan/e_ad_conv.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define SAMPLE_START 8000

#define SILENCE_CODES   400000
#define SILENCE_LETTERS 600000
#define SHORT_BEEPD  300
#define LONG_BEEPD   1200

extern int e_dci_unavailable;

void talk(int letters[], int letter_count) {
    e_init_sound();
    e_ad_scan_off();
    stall_ms(500000);
    int i, j, current_letter;
    for (i = 0; i < letter_count; i++) {
        current_letter = letters[i];
        for (j = 0; j < CODE_LEN; j++) {
            if (alphabet[current_letter][j] < 0)
                break;

            while (e_dci_unavailable);

            if (alphabet[current_letter][j] == 1) {
                e_play_sound(SAMPLE_START, SHORT_BEEPD);    // short beep
            } else {
                e_play_sound(SAMPLE_START, LONG_BEEPD);     // long beep
            }

            stall_ms(SILENCE_CODES);
        }

        if (i < letter_count - 1)
            stall_ms(SILENCE_LETTERS);
    }
    e_close_sound();
}


#pragma clang diagnostic pop
