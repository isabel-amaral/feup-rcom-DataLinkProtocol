#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "transmitter.h"
#include "link_layer.h"
#include "utils.h"
#include "sup_tx_state_machine.h"

int alarm_enabled, alarm_count;
extern unsigned char control_rcv_tx;
int ns_tx, nr;

void alarm_handler(int signal) {
    alarm_enabled = 0;
    alarm_count++;
}

int tx_start_transmission(int fd) {
    alarm_enabled = 0;
    alarm_count = 0;
    (void) signal(SIGALRM, alarm_handler);
    printf("New alarm handler set\n");

    unsigned char* set_frame = assemble_supervision_frame(SET_CONTROL);

    while (alarm_count < 3) {
        if (!alarm_enabled) {
            write(fd, set_frame, SUP_FRAME_SIZE);
            printf("SET supervision frame sent\n");
            alarm(3);
            alarm_enabled = 1;
            if (!tx_state_machine(fd)) {
                ns_tx = 0;
                printf("UA supervision frame read\n");
                return 0;
            }   
        }
    }
    printf("Transmission failed\n");
    return 1;
}

int tx_stop_transmission(int fd) {
    alarm_enabled = 0;
    alarm_count = 0;
    (void) signal(SIGALRM, alarm_handler);
    printf("New alarm handler set\n");

    unsigned char* disc_frame = assemble_supervision_frame(DISC_CONTROL);
    unsigned char* ua_frame = assemble_supervision_frame(UA_CONTROL);

    while (alarm_count < 3) {
        if (!alarm_enabled) {
            write(fd, disc_frame, SUP_FRAME_SIZE);
            alarm(3);
            alarm_enabled = 1;
            printf("DISC supervision frame sent\n");
        }
        if (!tx_state_machine(fd) && control_rcv_tx == DISC_CONTROL) {
            printf("DISC supervision frame read\n");

            write(fd, ua_frame, SUP_FRAME_SIZE);
            printf("UA supervision frame sent\n");
            return 0;
        }
    }
    printf("Disconnection failed\n");
    return 1;
}

int send_info_frame(int fd, unsigned char* buffer, int buffer_size) {
    unsigned char control_field = assemble_info_frame_ctrl_field(ns_tx);
    int info_frame_size;
    unsigned char* info_frame = assemble_information_frame(control_field, buffer, buffer_size, &info_frame_size);

    int resend = 0;
    alarm_enabled = 0;
    alarm_count = 0;
    (void) signal(SIGALRM, alarm_handler);
    while (alarm_count < 3) { 
        if (!alarm_enabled || resend) {
            resend = 0;
            write(fd, info_frame, info_frame_size);
            printf("Information frame sent\n");
            alarm(3);
            alarm_enabled = 1;
        }

        if (!tx_state_machine(fd)) { 
            nr = control_rcv_tx & BIT(7);
            if ((control_rcv_tx & RR_ACK) == RR_ACK && nr != ns_tx)
                break;
            else if ((control_rcv_tx & RR_ACK) == RR_ACK && nr == ns_tx) {
                resend = 1;
            }
            else if ((control_rcv_tx & REJ_ACK) == REJ_ACK) {
                alarm(3);
                alarm_enabled = 0;
            }
        }
    }

    ns_tx = (ns_tx == 0) ? 1 : 0;
    return 0;
}
