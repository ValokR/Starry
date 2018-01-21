#include <cstdlib>
#include <string>
#include <algorithm>
#include "connection.h"

#define MAX_BUFFER_SIZE 10000 // max payload size of 10KiB
#define PAYLOAD_TOO_LARGE 32

class connection;

// forward declarations
void parse_message();

void ping();

void get_stats();

void reset_stats();

void error(int error_code);

void compression();

using namespace std;

// set up all the data **************************
char buffer[MAX_BUFFER_SIZE];
int sockfd;
connection connection_socket;
string magic_value;
string payload_length;
string code;
string magic_value_string = "53545259";
string payload;
int int_code = 0;
int int_payload_length = 0;
int total_bytes_received = 0;
int total_bytes_sent = 0;
/*************************************************/

int main() {

    // set up the socket file descriptor
    sockfd = connection_socket.setup_connection();

    // retreive the incoming data until successful
    connection_socket.receive_data(sockfd, buffer, MAX_BUFFER_SIZE);

    string message = buffer;

    // parse the message from the buffer
    parse_message();

    if (magic_value != magic_value_string) {
        perror("Invalid Magic Number");
    }

    if (int_payload_length > MAX_BUFFER_SIZE) {
        code = PAYLOAD_TOO_LARGE;
    }

    switch (int_code) {
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
        default:
            break;
    }
}

void parse_message() {
    // copy first 4 bytes of buffer, which should be 53545259
    for (int i = 0; i < 8; i++) {
        magic_value += buffer[i];
    }

    // copy payload length
    for (int j = 8; j < 12; j++) {
        payload_length += buffer[j];
    }
    int_payload_length = std::stoi(payload_length, nullptr, 10);

    // copy code
    for (int k = 12; k < 16; k++) {
        code += buffer[k];
    }
    int_code = std::stoi(code, nullptr, 10);

    //copy payload_length bytes
    for (int l = 15; l < (int_payload_length + 16); l++) {
        payload += buffer[l];
    }

    // if payload is too large, send appropriate response
    if (int_payload_length > MAX_BUFFER_SIZE) {
        error(2);
    }

    total_bytes_received += (8 + int_payload_length);
}

void ping() {
    // copy magic number into buffer
    for (int i = 0; i < 8; i++) {
        buffer[i] = magic_value_string.at(i);
    }

    // set payload length to 0
    for (int j = 8; j < 12; j++) {
        buffer[j] = payload_length.at(j - 8);      // set payload length
    }

    // copy OK code (0) into code field
    code = "0000";
    for (int k = 12; k < 16; k++) {
        buffer[k] = code.at(k - 12);
    }

    // send the message
    connection::send_data(sockfd, buffer, 16);
    total_bytes_sent += 8;

}

void error(int error_code) {
    // copy magic number
    for (int i = 0; i < 8; i++) {
        buffer[i] = magic_value_string.at(i);
    }

    // set payload length to 0
    for (int j = 8; j < 12; j++) {
        buffer[j] = payload_length.at(j - 8);      // set payload length
    }

    // set proper response code
    code = std::to_string(error_code);

    // pad code with proper amount of zeros
    while (code.length() < 4) {
        code = "0" + code;
    }

    for (int k = 12; k < 16; k++) {
        buffer[k] = code.at(k - 12);
    }

    // send the message
    connection::send_data(sockfd, buffer, 16);
    total_bytes_sent += 8;
    exit(0);
}

void get_stats() {
    // convert int to string for processing
    string bytes_received = std::to_string(total_bytes_received);

    // prepend with proper amount of 0s
    while (bytes_received.length() < 8) {
        bytes_received = "0" + bytes_received;
    }

    // load data into buffer to be sent
    for (int i = 0; i < 8; ++i) {
        buffer[i] = bytes_received.at(i);
    }

    // convert int to string for processing
    string bytes_sent = std::to_string(total_bytes_sent);

    // prepend with proper amount of 0s
    while (bytes_sent.length() < 8) {
        bytes_sent = "0" + bytes_sent;
    }

    // load data into buffer to be sent
    for (int j = 8; j < 16; ++j) {
        buffer[j] = bytes_sent.at(j - 8);
    }

    // calculate compression ratio, and add to output buffer
    int compression_ratio = (total_bytes_sent / total_bytes_received) * 100;

    // convert compression ratio to char
    string str_compression_ratio = to_string(compression_ratio);

    // load data into buffer to be sent
    buffer[16] = str_compression_ratio.at(0);
    buffer[17] = str_compression_ratio.at(1);

    // send the message
    connection::send_data(sockfd, buffer, 18);
    total_bytes_sent += 9;
}

void reset_stats() {
    // copy magic number
    for (int i = 0; i < 8; i++) {
        buffer[i] = magic_value_string.at(i);
    }

    // set payload length to 0
    for (int j = 8; j < 12; j++) {
        buffer[j] = payload_length.at(j - 8);      // set payload length
    }

    // copy OK code (0) into code field
    code = "0000";
    for (int k = 12; k < 16; k++) {
        buffer[k] = code.at(k - 12);
    }

    // send the message
    connection::send_data(sockfd, buffer, 16);
    total_bytes_sent += 8;
}

void compression() {
    char current_char = payload.at(0);  // first value in payload
    int consecutive_chars = 1;          // temp variable to hold # of times a char is repeated
    string payload_builder;             // string to build up compressed payload with

    for (int i = 1; i < payload.length(); i++) {
        if (current_char ==
            payload.at(i)) {            // if it matches, consecutive detected.  Increment by 1 and examine next element
            consecutive_chars++;
        } else {                        // new character detected, build string for previous char, then move to next
            if (consecutive_chars > 1) {
                payload_builder = payload_builder + std::to_string(consecutive_chars);
            }
            payload_builder = payload_builder + current_char;
            current_char = payload.at(i);
            consecutive_chars = 1;
        }
    }

    // copy magic number
    for (int i = 0; i < 8; i++) {
        buffer[i] = magic_value_string.at(i);
    }

    // set up payload length
    payload_length = sizeof(payload_builder);
    while (payload_length.length() < 5) {
        payload_length = "0" + payload_length;
    }

    // copy payload length
    for (int j = 8; j < 12; j++) {
        buffer[j] = payload_length.at(j - 8);      // set payload length
    }

    // copy payload
    for (int k = 12; k < 12 + payload_builder.length(); k++) {
        buffer[k] = payload_builder.at(k - 12);
    }

    connection::send_data(sockfd, buffer, static_cast<int>(12 + payload_builder.length()));
    total_bytes_sent += 6 + (payload_builder.length() / 2);
}