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

void talk(char *word, int max_word_len) {

    //e_ad_scan_off();
    staller(1);

    int i;
    for (i = 0; i < max_word_len; i++) {
        // while (e_dci_unavailable);
        if (word[i] == '2') {
            e_play_sound(SAMPLE_START, LONG_BEEPD);     // long beep
        }
        if (word[i] == '1') {
            e_play_sound(SAMPLE_START, SHORT_BEEPD);    // short beep
        }
        if (word[i] == '0') {
            break;
        }
        stall_ms(SILENCE_CODES);
    }

    // e_close_sound();
    stall_ms(SILENCE_LETTERS);
}


#pragma clang diagnostic pop
