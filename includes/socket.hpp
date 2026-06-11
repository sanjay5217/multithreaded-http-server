#pragma once 

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

const int CONST_BUFFER_SIZE = 4096;

class Socket {
    private:
    int fd;
    std::string buffer;
    int buffer_size;

    public:

    /**
     * @brief Constructs a socket
     *
     * @param socket_fd: the file descriptor for the socket
     */
    Socket(int socket_fd);

    /**
     * @brief Parses raw bytes from the socket into a string 
     *
     * @return The parsed string 
     */
    std::string get_message(void);

    /**
     * @brief Reads exactly <length> amount of bytes into a string
     *
     * @param length The amount of bytes to read
     * @return The parsed string 
     */
    std::string get_message(int length);

    /**
     * @brief Writes the string message into the socket
     *
     * @param msg: The string message to be written
     * @return True if the write was successful and False otherwise
     */
    bool send_message(std::string msg);

    /**
     * @brief Closes the socket and deconstructs it
     */
    ~Socket();
};