#include <unistd.h>
#include <stdio.h>

#include "sup_tx_state_machine.h"
#include "link_layer.h"

enum State state_tx;
unsigned char control_rcv_tx;

void tx_start_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == FLAG)
        state_tx = FLAG_RCV;
}

void tx_flag_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == ADDRESS)
        state_tx = A_RCV;
    else if (byte_rcv != FLAG)
        state_tx = START;
}

void tx_a_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == UA_CONTROL || byte_rcv == DISC_CONTROL ||
        byte_rcv == RR_ACK ||  byte_rcv == (unsigned char) (RR_ACK | SET_SUP_FRAME_CONTROL) ||
        byte_rcv == REJ_ACK || byte_rcv == (unsigned char) (REJ_ACK | SET_SUP_FRAME_CONTROL)) {
        state_tx = C_RCV;
        control_rcv_tx = byte_rcv;
    }
    else if (byte_rcv == FLAG)
        state_tx = FLAG_RCV;
    else
        state_tx = START;
}

void tx_c_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == (ADDRESS ^ control_rcv_tx))
        state_tx = BCC_OK;
    else if (byte_rcv == FLAG)
        state_tx = FLAG_RCV;
    else
        state_tx = START;
}

void tx_bcc_ok_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == FLAG)
        state_tx = STOP;
    else
        state_tx = START;
}

int tx_state_machine(int fd) {
    unsigned char byte_rcv[BYTE_SIZE];
    state_tx = START;
    if (!read(fd, byte_rcv, BYTE_SIZE)) 
        return 1;
    if (byte_rcv[0] != FLAG)
        return 1;

    while (state_tx != STOP) {
        if (state_tx != START)
            read(fd, byte_rcv, BYTE_SIZE);

        switch (state_tx) {
        case START:
            tx_start_transition_check(byte_rcv[0]); break;
        case FLAG_RCV:
            tx_flag_rcv_transition_check(byte_rcv[0]); break;
        case A_RCV:
            tx_a_rcv_transition_check(byte_rcv[0]); break;
        case C_RCV:
            tx_c_rcv_transition_check(byte_rcv[0]); break;
        case BCC_OK:
            tx_bcc_ok_transition_check(byte_rcv[0]); break;
        default:
            break;
        }
    }
    return 0;
}
