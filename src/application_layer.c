#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application_layer.h"
#include "link_layer.h"
#include "utils.h"

int send_data(int fd, unsigned char* data, int file_size) {
    int sequence_number = 0;
    int data_field_size, packet_size;
    unsigned char data_field[DATA_FIELD_BYTES];
    unsigned char packet[DATA_FIELD_BYTES + 4];

    for (long i = 0; i < file_size; i += PACKET_DATA_FIELD_SIZE) {
        data_field_size = (i + PACKET_DATA_FIELD_SIZE > file_size)  ? file_size - i : PACKET_DATA_FIELD_SIZE;
        memcpy(data_field, data + i, data_field_size);

        packet_size = data_field_size + 4;
        assemble_data_packet(sequence_number, data_field, data_field_size, packet);
        if (llwrite(fd, packet, packet_size))
            return 1;

        sequence_number = (sequence_number + 1) % DATA_PACKET_MAX_SIZE;
    }
    return 0;
}

int send_file(int fd, const char* filename) {
    FILE* fptr;
    if (!(fptr = fopen(filename, "r")))
        return 1;

    fseek(fptr, 0, SEEK_END);
    long file_size = ftell(fptr);
    rewind(fptr);

    unsigned char* data = (unsigned char*) malloc(file_size);
    if (fread(data, sizeof(unsigned char), file_size, fptr) < file_size)
        return 1;
    
    if (send_control_packet(fd, CTRL_START, file_size, (unsigned char*) filename)) 
        return 1;
    if (send_data(fd, data, file_size))
        return 1;
    if (send_control_packet(fd, CTRL_END, file_size, (unsigned char*) filename)) 
        return 1;

    if (fclose(fptr))
        return 1;
    return 0;
}

int receive_file(int fd) {
    long file_size;
    unsigned char* src_filename = receive_control_packet(fd, CTRL_START, &file_size);
    
    unsigned char* data = (unsigned char*) malloc(file_size);
    unsigned char* data_ptr = data;
    unsigned char* packet = (unsigned char*) malloc(DATA_CTRL_PACK_SIZE); 
    int packet_size;
    memset(packet, 0, DATA_CTRL_PACK_SIZE); 

    packet_size = llread(fd, packet) - 4;
    while (packet[0] != CTRL_END) { 
        memcpy(data_ptr, packet + 4, packet_size);
        data_ptr += packet_size;
        packet_size = llread(fd, packet) - 4;
    }
    printf("File received\n");
    
    char* dest_filename = (char*) malloc(strlen("received_") + strlen((char*) src_filename) + 1);
    strcpy(dest_filename, "received_");
    strcat(dest_filename, (char*) src_filename);

    FILE* fptr;
    if (!(fptr = fopen(dest_filename, "w")))
        return 1;
    if (fwrite(data, sizeof(unsigned char), file_size, fptr) < file_size)
        return 1;
    if (fclose(fptr))
        return 1;
    return 0;
}

int application_layer(const char *serial_port, const char *role, const char *filename) {
    LinkLayer connection_parameters;
    strcpy(connection_parameters.serial_port, serial_port);

    LinkLayerRole link_layer_role;
    if (!strcmp(role, "tx"))
        link_layer_role = LlTx;
    else
        link_layer_role = LlRx;
    connection_parameters.role = link_layer_role;        

    int fd; 
    if ((fd = llopen(connection_parameters)) < 0)
        return 1;

    if (link_layer_role == LlTx && send_file(fd, filename))
        return 1;
    else if (link_layer_role == LlRx && receive_file(fd))
        return 1;

    if (llclose(fd, connection_parameters) < 0)
        return 1;
    return 0;
}
