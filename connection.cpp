#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <string>
#include "connection.h"

#define PORT "4000"
#define QUEUE 15
#define MAX_BUFFER_LENGTH 256

using namespace std;

int connection::setup_connection() {

        int status;
        int sockfd, new_fd;
        struct addrinfo hints, *servinfo, *p;
        struct sockaddr_storage incoming_addr;
        socklen_t addr_size;
        int yes = 1;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        // load up servinfo struct
        if ((status == getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo error: %s \n", gai_strerror(status));
            exit(1);
        }

        // set up connection file descriptor, loop through all results
        // loaded into servinfo from getaddrinfo() & catch errors
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("Client: connection");
                continue;
            }

            // release port number to avoid "Address already in use" & catch errors
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
                perror("setsockopt");
                exit(1);
            }

            // bind the connection file descriptor to a specific port & catch errors
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            }

            break;
        }

        // no longer using servinfo struct, free up space
        freeaddrinfo(servinfo);

        // No bind?  Catch error
        if (p == NULL) {
            fprintf(stderr, "server: failed to bind \n");
            exit(1);
        }

        // listen & catch errors
        if (listen(sockfd, QUEUE) == -1) {
            perror("listen");
            exit(1);
        }

        // accept & catch errors
        addr_size = sizeof(incoming_addr);
        new_fd = accept(sockfd, (struct sockaddr *) &incoming_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            exit(1);
        } else {
            printf("Connection Established");
        }

        return new_fd;
    }

    int connection::send_data(int sockfd, char *output, int message_length) {
        int total = 0;                          // bytes sent
        int bytes_remaining = message_length;   // remaining bytes to be sent
        int n;

        while (total < message_length) {
            n = send(sockfd, (output + total), bytes_remaining, 0);
            if (n == 1) {
                break;
            }
            total += n;
            bytes_remaining -= n;
        }

        message_length = total;    // number of bytes sent successfully
        return (n == -1) ? -1 : 0;
    }

    int connection::receive_data(int sockfd, char *incoming_stream, int max_buffer_length) {
        int n = recv(sockfd, incoming_stream, MAX_BUFFER_LENGTH, 0);
        if (n == -1) {
            perror("receive");
        }

        return n;
    }

