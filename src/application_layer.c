#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application_layer.h"
#include "link_layer.h"

int send_file(int fd, const char* filename) {
    FILE* fptr;
    if (!(fptr = fopen(filename, "r")))
        return 1;

    fseek(fptr, 0, SEEK_END);
    long file_size = ftell(fptr);
    rewind(fptr);

    char* data = (char*) malloc(file_size);
    if (fread(data, sizeof(char), file_size, fptr) < file_size)
        return 1;

    // TODO
    // send control packet
    // send data
    // send control packet

    if (fclose(fptr))
        return 1;
    return 0;
}

int receive_file(int fd) {
    char src_filename[1] = "a"; // TODO: remove initialization
    long file_size = 0; // TODO: remove initialization

    // TODO
    // read control packet: will give us the file size and name
    
    char* data = (char*) malloc(file_size);
    /* Commented while llwrite not working
    char* data_ptr = data;
    char* packet = (char*) malloc(256); // TODO: change to macro
    int packet_size;
    memset(packet, 0, 256); // TODO: change to macro

    while (packet[0] != 3) { // TODO: change to macro
        llread(fd, packet);
        packet_size = 256 * packet[2] + packet[3]; // TODO: change to macro
        memcpy(data_ptr, packet + 4, packet_size);
        data_ptr += packet_size;
    }*/
    
    char* dest_filename = (char*) malloc(sizeof("received_") + sizeof(src_filename) - 1);
    strcpy(dest_filename, "received_");
    strcat(dest_filename, src_filename);

    FILE* fptr;
    if (!(fptr = fopen(dest_filename, "w")))
        return 1;
    if (fwrite(data, sizeof(char), file_size, fptr) < file_size)
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