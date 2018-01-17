#include <iostream>

#ifndef STARRY_CONNECTION_H
#define STARRY_CONNECTION_H

class connection;

class connection {
public:
    int setup_connection();

    static int send_data(int sockfd, char *output, int *message_length);

    void receive_data(int sockfd, char *incoming_stream, int max_buffer_length);
};

#endif //STARRY_CONNECTION_H
