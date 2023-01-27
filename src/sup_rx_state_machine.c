#include <unistd.h>

#include "sup_rx_state_machine.h"
#include "link_layer.h"

enum State state_rx;
unsigned char control_rcv_rx;

void rx_start_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == FLAG)
        state_rx = FLAG_RCV;
}

void rx_flag_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == ADDRESS)
        state_rx = A_RCV;
    else if (byte_rcv != FLAG)
        state_rx = START;
}

void rx_a_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == SET_CONTROL || byte_rcv == UA_CONTROL || byte_rcv == DISC_CONTROL) {
        state_rx = C_RCV;
        control_rcv_rx = byte_rcv;
    }
    else if (byte_rcv == FLAG)
        state_rx = FLAG_RCV;
    else
        state_rx = START;
}

void rx_c_rcv_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == (ADDRESS ^ control_rcv_rx))
        state_rx = BCC_OK;
    else if (byte_rcv == FLAG)
        state_rx = FLAG_RCV;
    else
        state_rx = START;
}

void rx_bcc_ok_transition_check(unsigned char byte_rcv) {
    if (byte_rcv == FLAG)
        state_rx = STOP;
    else
        state_rx = START;
}

int rx_state_machine(int fd) {
    unsigned char byte_rcv[BYTE_SIZE];
    state_rx = START;
    while (state_rx != STOP) {
        read(fd, byte_rcv, BYTE_SIZE);

        switch (state_rx) {
        case START:
            rx_start_transition_check(byte_rcv[0]); break;
        case FLAG_RCV:
            rx_flag_rcv_transition_check(byte_rcv[0]); break;
        case A_RCV:
            rx_a_rcv_transition_check(byte_rcv[0]); break;
        case C_RCV:
            rx_c_rcv_transition_check(byte_rcv[0]); break;
        case BCC_OK:
            rx_bcc_ok_transition_check(byte_rcv[0]); break;
        default:
            break;
        }
    }
    return 0;
}
