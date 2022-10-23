#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#include "utils.h"
#include "link_layer.h"

int stuffing(char* data, char* stuffed_data, int length) {
    int stuffed_data_size = 0;
    char* stuffed_data_ptr = stuffed_data;

    for (int i = 0; i < length; i++) {
        if (*data == FLAG || *data == ESCAPE) {
            *stuffed_data_ptr = ESCAPE;
            stuffed_data_ptr++;
            stuffed_data_size++;
            *stuffed_data_ptr = *data ^ STF_XOR;
        }
        else
            *stuffed_data_ptr = *data;

        data++;
        stuffed_data_ptr++;
        stuffed_data_size++;
    }
    return stuffed_data_size;
}

char generate_bcc2(const char* data_rcv) {
    char bcc2 = data_rcv[0];
    for (int i = 1; i < DATA_FIELD_BYTES; i++)
        bcc2 ^=  data_rcv[i];
    return bcc2;
}

void assemble_data_packet(int sequence_number, char* data, int data_size, char* packet) {
    packet[CONTROL_IDX] = CTRL_FIELD;
    packet[SEQUENCE_NUM_IDX] = sequence_number;
    packet[L1_IDX] = data_size / DATA_PACKET_MAX_SIZE;
    packet[L2_IDX] = data_size % DATA_PACKET_MAX_SIZE;
    memcpy (packet + DATA_FIELD_START_IDX, data, data_size + 4);
}

char* assemble_supervision_frame(char control_field) {
    char* sup_frame = malloc(SUP_FRAME_SIZE);
    sup_frame[FLAG1_IDX] = FLAG;
    sup_frame[ADDRESS_IDX] = ADDRESS;
    sup_frame[CONTROL_IDX] = control_field;
    sup_frame[BCC1_IDX] = ADDRESS ^ control_field;
    sup_frame[S_FLAG2_IDX] = FLAG;

    return sup_frame;
}

char* assemble_information_frame(char control_field, char* buffer, int buffer_size, int* info_frame_size) {
    char* stuffed_data = (char*) malloc(buffer_size * 2);
    int stuffed_data_size = stuffing(buffer, stuffed_data, buffer_size);
    int frame_size = stuffed_data_size + 6;

    char* info_frame = malloc(frame_size);
    info_frame[FLAG1_IDX] = FLAG;
    info_frame[ADDRESS_IDX] = ADDRESS;
    info_frame[CONTROL_IDX] = control_field;
    info_frame[BCC1_IDX] = ADDRESS ^ control_field;

    for (int i = 0; i < stuffed_data_size; i++) {
        info_frame[DATA_START_IDX + i] = *stuffed_data;
        stuffed_data++;
    }

    info_frame[BCC2_IDX(stuffed_data_size)] = generate_bcc2(buffer);
    info_frame[I_FLAG2_IDX(stuffed_data_size)] = FLAG;

    *info_frame_size = frame_size;
    return info_frame;
}

char assemble_info_frame_ctrl_field(int ns) {
    char control_field = INFO_FRAME_CONTROL;
    if (ns)
        control_field |= SET_INFO_FRAME_CONTROL;
    return control_field;
}

char assemble_rr_frame_ctrl_field(int ns) {
    char control_field = RR_ACK;
    if (ns)
        control_field |= SET_SUP_FRAME_CONTROL;
    return control_field;
}

char assemble_rej_frame_ctrl_field(int ns) {
    char control_field = REJ_ACK;
    if (ns)
        control_field |= SET_SUP_FRAME_CONTROL;
    return control_field;
}

int create_termios_structure(int fd, const char* serialPortName) {
    if (fd < 0) {
        perror(serialPortName);
        return 1;
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        return 1;
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));
    
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 30; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;   // Blocking read until 5 chars received

    tcflush(fd, TCIOFLUSH);
    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("New termios structure set\n");
    return 0;
}
