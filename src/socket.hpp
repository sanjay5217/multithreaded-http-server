#pragma once 

#include <string>
#include <iostream>

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
    Socket(int socket_fd);
    ~Socket();
    std::string get_message(void);
    bool send_message(std::string);
};

