#include "motor_led/advance_one_timer/e_motors.h"

#include <motor_led/e_epuck_ports.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_motors.h>
#include <motor_led/advance_one_timer/e_agenda.h>

void rotate_bot() {
    e_init_motors();
    e_start_agendas_processing();

    e_set_speed_left(500);
    e_set_speed_right(-500);
}
