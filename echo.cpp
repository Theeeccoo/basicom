#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)
#define BUFFER_SIZE 1024

struct sockaddr_in address;
socklen_t address_len = sizeof(address);

int init_server_socket(void)
{
    int PORT = 8080;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( sockfd == -1 ) handle_error("Socket Failed.");
    // if ( fcntl(sockfd, F_SETFL, O_NONBLOCK ) ) handle_error("Failed to set socket to non_blocking.");

    int optval = 1;
    if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval , sizeof(optval)) == - 1 ) handle_error("setsockopt Failed.");

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if ( bind(sockfd, (struct sockaddr *) &address, sizeof(address)) == -1 ) handle_error("Bind Failed.");

    return sockfd;
}

int main(void)
{
    int sockfd = init_server_socket();
    int clientfd;
    char buffer[BUFFER_SIZE];

    //                    4096
    if ( listen(sockfd, SOMAXCONN) == - 1 ) handle_error("Listen Failed.");
    if ( (clientfd = accept(sockfd, (struct sockaddr *) &address, &address_len )) == -1) handle_error("Accept Failed.");
    std::cout << "Connected: " << clientfd << std::endl;

    ssize_t amount_read    = 0;
    ssize_t amount_written = 0;

    while(true)
    {
        if ( (amount_read - amount_written) == 0 )
        {
            memset(&buffer, 0, BUFFER_SIZE);
            amount_read = read(clientfd, buffer, BUFFER_SIZE);
        }



        if ( amount_read > 0 )
        {
            std::cout << "SAID: " << buffer << std::endl;
            amount_written = write(clientfd, buffer, amount_read);
        }
    }

    close(clientfd);
    close(sockfd);
    return 0;
}
