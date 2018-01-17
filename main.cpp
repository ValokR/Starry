#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <zlib.h>
#include <algorithm>
#include "connection.h"

#define MAGIC_VALUE 53545259
#define MAX_BUFFER_SIZE 10000 // max payload size of 10KiB
#define CODE_SIZE 2           // 2 bytes
#define PAYLOAD_TOO_LARGE 32

class connection;

// forward declarations
void parse_message();
void ping();
void get_stats() ;
void reset_stats() ;
void error(int error_code) ;
void compression() ;

using namespace std;

// set up all the data **************************
char buffer[MAX_BUFFER_SIZE];
char out_buffer[MAX_BUFFER_SIZE];
int sockfd;
const void *magic_value_string = "53545259";
connection connection_socket;
uint32_t magic_value;
uint16_t payload_length;
uint16_t code;
uint32_t total_bytes_received = 0;
uint32_t total_bytes_sent = 0;
string payload;
int request_codes[] = {1, 2, 3, 32};
/*************************************************/


int main(void) {
    // set up the socket file descriptor
    sockfd = connection_socket.setup_connection();

    // while loop keeps server looking for incoming data
    while(1) {
        // retrieve the incoming data
        connection_socket.receive_data(sockfd, buffer, MAX_BUFFER_SIZE);

        string message = buffer;

        // parse the message from the buffer
        parse_message();

        if (magic_value != MAGIC_VALUE) {
            perror("Invalid Magic Number");
        }

        if (payload_length > MAX_BUFFER_SIZE) {
            code = PAYLOAD_TOO_LARGE;
        }

        switch(code) {
            case 1:
                ping();
                break;
            case 2:
                get_stats();
                break;
            case 3:
                reset_stats();
                break;
            case 4:
                compression();
                break;
        }

    }
}

// ********************************************************************************************************************
void parse_message() {
    // copy first 4 bytes of buffer, which should be 53545259 (from 0x53545259)
    memcpy(&magic_value, &buffer, sizeof(magic_value));

    // copy first 2 bytes starting at 4 (after magic value)
    memcpy(&payload_length, &buffer + 4, sizeof(payload_length));

    // copy first 2 elements starting at 6 (after magic value and payload)
    memcpy(&code, &buffer + 6, sizeof(code));

    //copy payload_length bytes, starting at 8 (after magic value, payload_length and code)
    memcpy(&payload, &buffer + 8, payload_length);

    // if payload is too large, send appropriate response
    if (payload_length > MAX_BUFFER_SIZE) {
        error(2);
    } else if (std::find(std::begin(request_codes), std::end(request_codes), code)) {
        error(3);
    }

    // received 8 bytes, add this to total received
    total_bytes_received += (8 + payload_length);
}

void ping() {
    // copy magic number into output buffer
    memcpy(&out_buffer, &magic_value_string, sizeof(magic_value_string));

    // copy empty payload length into output buffer, starting at index 7
    memcpy(&out_buffer + 4, "0", 1);

    // copy OK code (0) into code field
    memcpy(&out_buffer + 6, "0000", CODE_SIZE);

    // send the message
    connection::send_data(sockfd, out_buffer, reinterpret_cast<int *>(8));
    total_bytes_sent += 8;

}

void error(int error_code) {
    // copy magic number
    memcpy(&out_buffer, &magic_value_string, sizeof(magic_value_string));

    // set payload length to 0
    memcpy(&out_buffer + 4, "0", 1);

    // set proper response code
    memcpy(&out_buffer + 5, &error_code, sizeof(error_code));
}

void get_stats() {
    // copy bytes received to output buffer
    memcpy(&out_buffer, &total_bytes_received, sizeof(total_bytes_received));

    // copy bytes sent to output buffer
    memcpy(&out_buffer + 4, &total_bytes_sent, sizeof(total_bytes_sent));

    // calculate compression ratio, and add to output buffer
    uint8_t compression_ratio = static_cast<uint8_t>((total_bytes_received / total_bytes_sent) * 100);
    memcpy(&out_buffer + 8, &compression_ratio, sizeof(compression_ratio));

    // send the message
    connection::send_data(sockfd, out_buffer, reinterpret_cast<int *>(9));
    total_bytes_sent += 9;
}

void reset_stats() {
    // copy magic number into output buffer
    memcpy(&out_buffer, &magic_value_string, sizeof(magic_value_string));

    // copy status code to ouput buffer
    memcpy(&out_buffer + 4, "0000", CODE_SIZE);

    // send the message
    connection::send_data(sockfd, out_buffer, reinterpret_cast<int *>(6));
    total_bytes_sent += 6;
}

void compression() {
    char current_char = payload.at(0);  // first value in payload
    int consecutive_chars = 1;          // temp variable to hold # of times a char is repeated
    string payload_builder;             // string to build up compressed payload with

    for (int i = 1; i < sizeof(payload); i++) {
        if (current_char == payload.at(i)) {     // if it matches, consecutive detected.  Increment by 1 and examine next element
            consecutive_chars += 1;
        } else {                                 // new character detected, build string for previous char, then move to next
            payload_builder += std::to_string(consecutive_chars + current_char);
            current_char = payload.at(i);
        }
    }

    payload_length = sizeof(payload_builder);

    // after compression, send message out
    memcpy(&out_buffer, &magic_value_string, sizeof(magic_value_string));
    memcpy(&out_buffer + 4, &payload_length, sizeof(payload_length));
    memcpy(&out_buffer + 6, &payload_builder, sizeof(payload_builder));

    connection::send_data(sockfd, out_buffer, reinterpret_cast<int *>(6 + sizeof(payload_builder)));
    total_bytes_sent += 6 + sizeof(payload_builder);
}

//TODO Implement response codes and set proper buffer for processing
//TODO Check for only lower case letter, reject otherwise