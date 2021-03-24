#include "stdio.h"
#include "string.h"

#include "epuck_ports.h"
#include "a_d/advance_ad_scan/e_ad_conv.h"
#include "my_utils.h"
#include "uart/e_uart_char.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define AVG_CNT 9
#define NOISE_RATIO 0.005
#define SIL_DUR 10
#define LST_AV_CT 4
#define EPS_ERR 3
#define L_DUR 18
#define S_DUR 4
#define STATE_NODE_LIMIT 60

extern int e_mic_scan[3][MIC_SAMP_NB]; // Array to store the mic values
extern unsigned int e_last_mic_scan_id; // ID of the last scan in the mic array
extern unsigned char is_ad_acquisition_completed; // Check if the acquisition is done

int last_avg = 0;
int not_evt_cnt = 0;
int last_avgs[LST_AV_CT];
int last_avgs_idx = 0;
int last_state = 0;

int dur = 0;
int is_first = 1;
int d_cnt = 0;
int watch_dog = 0;
char err_message[50];
int state_node_count = 0;

struct state {
    char is_on;
    int offset;
};

struct state states[STATE_NODE_LIMIT];

void append_state_node(struct state s) {
    if (state_node_count == STATE_NODE_LIMIT) {
        return;
    }
    states[state_node_count].offset = s.offset;
    states[state_node_count].is_on = s.is_on;
    state_node_count++;
}

int tidy_signal(void) {
    while (!e_ad_is_array_filled());
    long avg = 0;
    int i;
    struct state st;
    for (i = 0; i < AVG_CNT; i++) {
        avg += e_mic_scan[0][i];
    }
    avg = avg / AVG_CNT; // avg of first N samples

    if (last_avg == 0) {
        last_avg = avg;
    }

    if (is_first == 0) {
        if (dur > 200) {
            // long silence
            LED1 = 1;
            return 0;
        } else if (dur > 80) {
            // short silence
        }
    }

    if (watch_dog > 500) {
        sprintf(err_message, "watch dog timeout A\n");
        e_send_uart1_char(err_message, strlen(err_message));
        while (e_uart1_sending());
        stall_ms(10);
        return 0;
    }

    if (last_avg + last_avg * NOISE_RATIO < avg || last_avg - last_avg * NOISE_RATIO > avg) {
        // event detected
        if (last_state == 0) {
            LED0 = 1;
            st.is_on = '1';
            st.offset = dur;
            if (is_first) {
                st.offset = 0;
                is_first = 0;
            }
            append_state_node(st);
            d_cnt++;
            dur = 0;
            watch_dog = 0;
            last_state = 1;
        }
    } else { // not event
        last_avgs[last_avgs_idx] = avg;
        last_avgs_idx++;
        if (last_avgs_idx == LST_AV_CT) {
            last_avgs_idx = 0;
        }

        not_evt_cnt++;
        if (not_evt_cnt == SIL_DUR) {
            not_evt_cnt = 0;
            avg = 0;
            for (i = 0; i < LST_AV_CT; i++) {
                avg += last_avgs[i];
            }
            avg = avg / LST_AV_CT;
            last_avg = avg;
            LED0 = 0;
            if (last_state == 1) {
                st.is_on = '0';
                st.offset = dur;
                append_state_node(st);
                d_cnt++;
                dur = 0;
                last_state = 0;
            }

        }
    }

    dur++;
    watch_dog++;
    return 1;
}

void get_m_code(char *m_code, int max_word_len) {
    int offset, on_dur, m_code_i, i;

    on_dur = 0;
    m_code_i = 0;

    for (i = 0; i < state_node_count; i++) {
        offset = states[i].offset;

        if (states[i].is_on == '0') {
            on_dur += offset;
        } else {
            if (offset <= EPS_ERR) {
                on_dur += offset;
            } else {
                if (on_dur >= L_DUR) {
                    m_code[m_code_i] = '2';
                    m_code_i++;
                } else if (on_dur >= S_DUR) {
                    m_code[m_code_i] = '1';
                    m_code_i++;
                } else {
                    //return -1;
                    // m_code[m_code_i] = 1;
                }
                on_dur = 0;
                // m_code_i++;
                if (m_code_i == max_word_len) {
                    break;
                }
            }
        }

    }

    if (m_code_i < max_word_len) {
        if (on_dur >= L_DUR) {
            m_code[m_code_i] = '2';
        } else if (on_dur >= S_DUR) {
            m_code[m_code_i] = '1';
        } else {
            //return -1;
            // m_code[m_code_i] = 0;
            return;
        }
    }
}

void init_listening() {
    dur = 0;
    is_first = 1;
    watch_dog = 0;
    state_node_count = 0;
    d_cnt = 0;
    last_avg = 0;
    not_evt_cnt = 0;
    last_avgs_idx = 0;
    last_state = 0;
    e_last_mic_scan_id = 0;

    LED0 = 0;
    LED1 = 0;
    LED2 = 0;

    e_ad_scan_on();
    e_init_ad_scan(MICRO_ONLY);

    stall_ms(300000);
}

void listen(char *heard_word, int max_word_len) {
    init_listening();
    LED2 = 1;
    while (tidy_signal());
    e_ad_scan_off();
    get_m_code(heard_word, max_word_len);
    LED2 = 0;
}

#pragma clang diagnostic pop
