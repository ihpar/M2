#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "epuck_ports.h"
#include "a_d/advance_ad_scan/e_ad_conv.h"
#include "a_d/advance_ad_scan/e_micro.h"
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


extern int e_mic_scan[3][MIC_SAMP_NB];                //Array to store the mic values
extern unsigned int e_last_mic_scan_id;                //ID of the last scan in the mic array
extern unsigned char is_ad_acquisition_completed;    //to check if the acquisition is done


char buffer_db[200];
int last_avg = 0;
int not_evt_cnt = 0;
int last_avgs[LST_AV_CT];
int last_avgs_idx = 0;
int last_state = 0;

int dur = 0;
int is_first = 1;
int d_cnt = 0;


struct state {
    int is_on;
    int offset;
};


struct node {
    struct state st;
    struct node *next;
};
struct node *start = NULL;


void insert_at_end(struct state x) {
    struct node *t, *temp;

    t = (struct node *) malloc(sizeof(struct node));

    if (start == NULL) {
        start = t;
        start->st.offset = x.offset;
        start->st.is_on = x.is_on;
        start->next = NULL;
        return;
    }

    temp = start;

    while (temp->next != NULL)
        temp = temp->next;

    temp->next = t;
    t->st.offset = x.offset;
    t->st.is_on = x.is_on;
    t->next = NULL;
}


void delete_ll() {
    struct node *temp;

    while (start != NULL) {
        temp = start;
        start = start->next;

        free(temp);
    }
}


void send_s_data() {
    sprintf(buffer_db, "C:%d\n", d_cnt);
    e_send_uart1_char(buffer_db, strlen(buffer_db));

    struct node *t;

    t = start;

    if (t == NULL) {
        return;
    }

    while (t != NULL) {
        sprintf(buffer_db, "%d, %d\n", t->st.is_on, t->st.offset);
        e_send_uart1_char(buffer_db, strlen(buffer_db));
        t = t->next;
    }

    e_send_uart1_char("done\n", 5);
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

    if (last_avg + last_avg * NOISE_RATIO < avg || last_avg - last_avg * NOISE_RATIO > avg) {
        // event detected
        if (last_state == 0) {
            LED0 = 1;
            st.is_on = 1;
            st.offset = dur;
            if (is_first) {
                st.offset = 0;
                is_first = 0;
            }
            insert_at_end(st);
            d_cnt++;
            dur = 0;
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
                st.is_on = 0;
                st.offset = dur;
                insert_at_end(st);
                d_cnt++;
                dur = 0;
                last_state = 0;
            }

        }
    }

    dur++;
    return 1;
}

int find_similarity(int r[], int m[]) {
    int i, score = 0;
    for (i = 0; i < CODE_LEN; i++) {
        if (r[i] == m[i]) {
            score += 1;
        } else {
            score -= 1;
        }
    }
    return score;
}

int get_m_code() {
    int idx, is_on, offset, on_dur, m_code_i, i, max_similarity, similarity;
    struct node *t;
    if (start == NULL || start->next == NULL) {
        return -1;
    }

    t = start->next;

    int m_code[CODE_LEN] = {-1, -1, -1, -1, -1, -1};

    on_dur = 0;
    m_code_i = 0;
    while (t != NULL) {
        is_on = t->st.is_on;
        offset = t->st.offset;

        if (is_on == 0) {
            on_dur += offset;
        } else {
            if (offset <= EPS_ERR) {
                on_dur += offset;
            } else {
                if (on_dur >= L_DUR) {
                    m_code[m_code_i] = 2;
                } else if (on_dur >= S_DUR) {
                    m_code[m_code_i] = 1;
                } else {
                    //return -1;
                    m_code[m_code_i] = 0;
                }
                on_dur = 0;
                m_code_i++;
            }
        }
        t = t->next;
    }

    if (on_dur >= L_DUR) {
        m_code[m_code_i] = 2;
    } else if (on_dur >= S_DUR) {
        m_code[m_code_i] = 1;
    } else {
        //return -1;
        m_code[m_code_i] = 0;
    }

    // sprintf(buffer_db, "M: %d, %d, %d, %d, %d, %d\n", m_code[0], m_code[1], m_code[2], m_code[3], m_code[4], m_code[5]);
    // e_send_uart1_char(buffer_db, strlen(buffer_db));

    max_similarity = -1;
    idx = 0;
    for (i = 0; i < ALPHABET_LEN; i++) {
        similarity = find_similarity(m_code, alphabet[i]);
        if (similarity > max_similarity) {
            max_similarity = similarity;
            idx = i;
        }
    }

    return idx;
}


void init() {
    dur = 0;
    is_first = 1;
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

int listen() {
    int letter_index = 0;
    init();
    LED2 = 1;
    while (tidy_signal());
    e_ad_scan_off();
    letter_index = get_m_code();
    // send_s_data();
    delete_ll();
    LED2 = 0;
    return letter_index;
}


#pragma clang diagnostic pop
