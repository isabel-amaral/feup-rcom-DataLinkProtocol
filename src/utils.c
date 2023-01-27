#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#include "utils.h"
#include "application_layer.h"
#include "link_layer.h"

int stuffing(unsigned char* data, unsigned char* stuffed_data, int data_size) {
    int stuffed_data_size = 0;
    unsigned char* stuffed_data_ptr = stuffed_data;

    for (int i = 0; i < data_size; i++) {
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

unsigned char generate_bcc2(const unsigned char* data_rcv, int data_size) {
    unsigned char bcc2 = data_rcv[0];
    for (int i = 1; i < data_size; i++)
        bcc2 ^= data_rcv[i];
    return bcc2;
}

int send_control_packet(int fd, unsigned ctrl_control_field, long file_size, const unsigned char* file_name) {
    unsigned char *control_packet = malloc (5 + sizeof(long) + strlen((char*) file_name) + 1);

    control_packet[PKT_CTRL_FIELD_IDX] = ctrl_control_field;
    control_packet[TYPE1_IDX] = TYPE_FILE_SIZE;
    memcpy(control_packet + 3, &file_size, sizeof(long));
    control_packet[LENGTH1_IDX] = (unsigned char) sizeof(long);

    control_packet[sizeof(long) + 3] = TYPE_FILE_NAME;
    control_packet[sizeof(long) + 4] = (unsigned char) strlen((char*) file_name) + 1;
    memcpy(control_packet + sizeof(long) + 5, file_name, strlen((char*) file_name) + 1);

    llwrite(fd, control_packet, 5 + sizeof(long) + strlen((char*) file_name) + 1);
    return 0;    
}

unsigned char* receive_control_packet(int fd, unsigned char control_field, long* file_size) {
    unsigned char type;
    unsigned char *control_packet = malloc(DATA_CTRL_PACK_SIZE);
    int size, length;
    unsigned char* file_name;
    
    size = llread(fd, control_packet);

    if (control_packet[0] != control_field)
        return NULL;

    for (int i = 1; i < size; i += length) {
        type = control_packet[i++];
        length = control_packet[i++];

        if (type == TYPE_FILE_SIZE)
            memcpy(file_size, control_packet + i, length);
        else if (type == TYPE_FILE_NAME) {
            file_name = malloc(length);
            memcpy(file_name, control_packet + i, length);
        }  
    }
    return file_name;
}

void assemble_data_packet(int sequence_number, unsigned char* data, int data_size, unsigned char* packet) {
    packet[CTRL_FIELD_IDX] = CTRL_DATA;
    packet[SEQUENCE_NUM_IDX] = sequence_number;
    packet[L1_IDX] = data_size % PACKET_DATA_FIELD_SIZE;
    packet[L2_IDX] = data_size / PACKET_DATA_FIELD_SIZE;

    memcpy(packet + DATA_FIELD_START_IDX, data, data_size);
}

unsigned char* assemble_supervision_frame(unsigned char control_field) {
    unsigned char* sup_frame = malloc(SUP_FRAME_SIZE);
    sup_frame[FLAG1_IDX] = FLAG;
    sup_frame[ADDRESS_IDX] = ADDRESS;
    sup_frame[CONTROL_IDX] = control_field;
    sup_frame[BCC1_IDX] = ADDRESS ^ control_field;
    sup_frame[S_FLAG2_IDX] = FLAG;

    return sup_frame;
}

unsigned char* assemble_information_frame(unsigned char control_field, unsigned char* buffer, int buffer_size, int* info_frame_size) {
    unsigned char* stuffed_data = (unsigned char*) malloc(buffer_size * 2);
    int stuffed_data_size = stuffing(buffer, stuffed_data, buffer_size);
    int frame_size = stuffed_data_size + 6;

    unsigned char* info_frame = malloc(frame_size);
    info_frame[FLAG1_IDX] = FLAG;
    info_frame[ADDRESS_IDX] = ADDRESS;
    info_frame[CONTROL_IDX] = control_field;
    info_frame[BCC1_IDX] = ADDRESS ^ control_field;

    for (int i = 0; i < stuffed_data_size; i++) {
        info_frame[DATA_START_IDX + i] = *stuffed_data;
        stuffed_data++;
    }

    info_frame[BCC2_IDX(stuffed_data_size)] = generate_bcc2(buffer, buffer_size);
    info_frame[I_FLAG2_IDX(stuffed_data_size)] = FLAG;

    *info_frame_size = frame_size;
    return info_frame;
}

unsigned char assemble_info_frame_ctrl_field(int ns) {
    unsigned char control_field = INFO_FRAME_CONTROL;
    if (ns)
        control_field |= SET_INFO_FRAME_CONTROL;
    return control_field;
}

unsigned char assemble_rr_frame_ctrl_field(int ns) {
    unsigned char control_field = RR_ACK;
    if (ns)
        control_field |= SET_SUP_FRAME_CONTROL;
    return control_field;
}

unsigned char assemble_rej_frame_ctrl_field(int ns) {
    unsigned char control_field = REJ_ACK;
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
