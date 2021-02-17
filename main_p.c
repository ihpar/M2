#include "stdio.h"
#include "string.h"
#include "p30f6014A.h"
#include "epuck_ports.h"
#include "uart/e_uart_char.h"
#include "uart/e_uart_char.h"
#include "motor_led/e_init_port.h"
#include "codec/e_sound.h"
#include "a_d/advance_ad_scan/e_ad_conv.h"
#include "a_d/advance_ad_scan/e_micro.h"

#include "my_utils.h"
#include "talk.h"
#include "listen.h"
#include "rotator.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(void) {

    e_init_port();
    e_init_uart1();
    staller(4);

    // rotate_bot();

    int letter = 2;
    int letter_list[1] = {letter};

    int first_robot = 1;
    if (first_robot) {
        talk(letter_list, 1);
        stall_ms(200000);
    }

    int cntr = 0;

    while (1) {
        letter = listen();
        letter = 19;
        letter_list[0] = letter;
        stall_ms(200000);
        talk(letter_list, 1);
        stall_ms(200000);
        cntr++;
        if (cntr == 10) {
            e_send_uart1_char("FIN\n", 4);
        }
    }

}

#pragma clang diagnostic pop
