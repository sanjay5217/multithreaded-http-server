#pragma once 

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


const int CONST_BUFFER_SIZE = 1024;

class Socket {
    private:
    int fd;
    std::string buffer;

    public:

    /*
     * Constructor for Socket class
    */
    Socket(int socket_fd);

    /*
    * Retrieves message written to <fd> and returns it as a string.
    * Returns empty string if no message exists
    */
    std::string get_message(void);

    
    std::string get_body();

    /*
     * sends message to <fd> and returns true if successful and false otherwise
    */
    bool send_message(std::string);

    /*
     * Destructor for Socket class
    */
    ~Socket();
};
