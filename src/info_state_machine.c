#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "info_state_machine.h"
#include "application_layer.h"
#include "link_layer.h"
#include "utils.h"

enum InfoState info_state;
unsigned char control_rcv;
int has_error, is_data_packet, is_escaped, data_idx, data_size, l2;

void info_start_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == FLAG)
        info_state = I_FLAG_RCV;
}

void info_flag_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == ADDRESS)
        info_state = I_A_RCV;
    else
        has_error = 1;
}

void info_a_rcv_transition_check(unsigned char byte_rcv, int ns) {
    unsigned char expected_rcv = assemble_info_frame_ctrl_field(ns);
    if (byte_rcv == expected_rcv)
        control_rcv = byte_rcv;
    else
        has_error = 2;
    info_state = I_C_RCV;
}

void info_c_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv != (ADDRESS ^ control_rcv))
        has_error = 1;
    info_state = BCC1_RCV;
}

void info_bcc1_rcv_transition_check(unsigned char byte_rcv, unsigned char* data_rcv) {
    if (data_idx == CTRL_FIELD_IDX && byte_rcv != CTRL_DATA)
        is_data_packet = 0;

    if (is_data_packet && data_idx == (L1_IDX+1))
        data_size = PACKET_DATA_FIELD_SIZE * data_rcv[L2_IDX] + data_rcv[L1_IDX] + 4;
    else if (!is_data_packet && data_idx == LENGTH1_IDX) {
        data_size = byte_rcv + 5;
        l2 = data_size - 1;
    }
    else if (!is_data_packet && data_idx == l2)
        data_size += byte_rcv;

    if (is_escaped) {
        data_rcv[data_idx] = byte_rcv ^ STF_XOR;
        data_idx++;
        is_escaped = 0;
    }
    else if (byte_rcv == ESCAPE)
        is_escaped = 1;
    else {
        data_rcv[data_idx] = byte_rcv;
        data_idx++;
    }

    if (data_idx == data_size) {
        info_state = DATA_RCV;
        return;
    }
}

void info_data_rcv_transition_check(unsigned char byte_rcv, unsigned char* data_rcv) {
    if (byte_rcv != generate_bcc2(data_rcv, data_idx)) {
        has_error = (has_error == 1) ? 1 : 3;
    }
    info_state = BCC2_RCV;
}

void info_bcc2_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv != FLAG) 
        has_error = (has_error == 0) ? 1 : has_error;
    info_state = I_STOP;
}

int info_frame_state_machine(int fd, int ns, unsigned char* data_rcv, int* data_rcv_size) {
    memset(data_rcv, 0, DATA_FIELD_BYTES);
    // has_error = 1 indicates an error in the frame's header
    // has_error = 2 indicates that the frame being received is the worng one (duplicated)
    // has_error = 3 indicates an error in the frame's data field
    info_state = I_START;
    has_error = 0;
    is_data_packet = 1;
    is_escaped = 0;
    data_idx = 0;
    data_size = DATA_FIELD_BYTES;

    unsigned char byte_rcv[BYTE_SIZE];
    while (info_state != I_STOP && has_error == 0) {
        read(fd, byte_rcv, BYTE_SIZE);

        switch (info_state) {
        case I_START: 
            info_start_transition_check(byte_rcv[0]); break;
        case I_FLAG_RCV:
            info_flag_rcv_transition_check(byte_rcv[0]); break;
        case I_A_RCV: 
            info_a_rcv_transition_check(byte_rcv[0], ns); break;
        case I_C_RCV:
            info_c_rcv_transition_check(byte_rcv[0]); break;
        case BCC1_RCV:
            info_bcc1_rcv_transition_check(byte_rcv[0], data_rcv); break;
        case DATA_RCV:
            info_data_rcv_transition_check(byte_rcv[0], data_rcv); break;
        case BCC2_RCV:
            info_bcc2_rcv_transition_check(byte_rcv[0]); break;
        default:
            break;
        }
    } 

    *data_rcv_size = data_size;
    printf("Information frame read (code %d)\n", has_error);
    return has_error;
}
