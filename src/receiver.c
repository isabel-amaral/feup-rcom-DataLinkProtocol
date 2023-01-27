#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "receiver.h"
#include "link_layer.h"
#include "utils.h"
#include "sup_rx_state_machine.h"
#include "info_state_machine.h"

int ns_rx;

int rx_start_transmission(int fd) {
    rx_state_machine(fd);
    printf("SET supervision frame read\n");

    unsigned char* ua_frame = assemble_supervision_frame(UA_CONTROL);
    write(fd, ua_frame, SUP_FRAME_SIZE);
    printf("UA supervision frame sent\n");

    ns_rx = 0;
    return 0;
}

int rx_stop_transmission(int fd) {
    rx_state_machine(fd);
    printf("DISC supervision frame read\n");

    unsigned char* disc_frame = assemble_supervision_frame(DISC_CONTROL);
    write(fd, disc_frame, SUP_FRAME_SIZE);
    printf("DISC supervision frame sent\n");

    rx_state_machine(fd);
    printf("UA supervision frame read\n");
    printf("Successfully disconnected\n");
    return 0;
}

int receive_info_frame(int fd, unsigned char* packet, int* packet_size) {
    unsigned char* data_rcv = (unsigned char*) malloc(DATA_FIELD_BYTES);
    unsigned char* acknowledgement = (unsigned char*) malloc(SUP_FRAME_SIZE);
    unsigned char control_field;
    int data_rcv_size;

    int has_error = info_frame_state_machine(fd, ns_rx, data_rcv, &data_rcv_size);
    if (!has_error) {
        ns_rx = (ns_rx == 0) ? 1 : 0;
        memcpy(packet, data_rcv, data_rcv_size);
        *packet_size = data_rcv_size;
    }
    if (!has_error || has_error == 2) {
        control_field = assemble_rr_frame_ctrl_field(ns_rx);
        printf("RR supervision frame sent\n");
    }
    else if (has_error == 3) {
        control_field = assemble_rej_frame_ctrl_field(ns_rx);
        printf("REJ supervision frame sent\n");
    }
    if (has_error != 1)
        acknowledgement = assemble_supervision_frame(control_field);

    write(fd, acknowledgement, SUP_FRAME_SIZE);
    return 0;
}
