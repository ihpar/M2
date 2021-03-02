#include "stdio.h"
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
#include "rotator.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma clang diagnostic ignored "-Wmissing-noreturn"


int main(void) {
    char message[50];
    char command[10];
    int i, c;

    int letter = 3;
    int letter_list[1] = {letter};

    e_init_port();
    e_init_uart1();

    staller(4);

    while (1) {
        i = 0;
        c = 0;
        do {
            if (e_getchar_uart1(&command[i])) {
                c = command[i];
                i++;
            }
        } while (((char) c != '\n') && ((char) c != '\x0d'));
        command[i] = '\0';

        switch (command[0]) {
            case 'i':
                // stay idle
                sprintf(message, "ok-i");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
                break;
            case 'l':
                // listen
                listen();
                sprintf(message, "ok-l");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
                break;
            case 's':
                // speak
                talk(letter_list, 1);
                stall_ms(200000);
                sprintf(message, "ok-s");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
                break;
            default:
                // stay idle
                sprintf(message, "ok-di");
                e_send_uart1_char(message, strlen(message));
                while (e_uart1_sending());
                break;
        }

    }

}

#pragma clang diagnostic pop
