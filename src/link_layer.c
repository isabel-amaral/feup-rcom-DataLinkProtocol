#include <fcntl.h>

#include "link_layer.h"
#include "utils.h"
#include "transmitter.h"
#include "receiver.h"

int llopen(LinkLayer connection_parameters) {
    int fd = open(connection_parameters.serial_port, O_RDWR | O_NOCTTY);
    if (create_termios_structure(fd, connection_parameters.serial_port)) 
        return -1;

    if (connection_parameters.role == LlRx && rx_start_transmission(fd)) 
        return -1;
    else if (connection_parameters.role == LlTx && tx_start_transmission(fd))
        return -1;
    return fd;
}

int llwrite(int fd, unsigned char* packet, int packet_size) {
    return send_info_frame(fd, packet, packet_size);
}

int llread(int fd, unsigned char* packet) {
    int packet_size = 0;
    if (receive_info_frame(fd, packet, &packet_size))
        return -1;
    // for (int i = 4; i < packet_size; i++)
    //     printf("%c\n", packet[i]);
    // printf("\n");
    return packet_size;
}

int llclose(int fd, LinkLayer connection_parameters) {
    if (connection_parameters.role == LlRx && rx_stop_transmission(fd))
        return -1;
    else if (connection_parameters.role == LlTx && tx_stop_transmission(fd))
        return -1;
    return 0;
}
