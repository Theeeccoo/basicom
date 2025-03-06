#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "coroutine.h"

#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

#define BUFFER_SIZE 1024

int client_read_buffer(int cfd, char *buffer)
{
    coroutine_sleep_read(cfd);
    int n = read(cfd, buffer, BUFFER_SIZE);
    if ( n > 0 )
    {
        // Removing \r\n
        buffer[n - 2] = '\0';
        n -=2;
    }
    return n;
}

int client_write_buffer(int cfd, void *buffer, int size)
{
    coroutine_sleep_write(cfd);
    int n = write(cfd, buffer, size);
    return n;
}

void client_echo(void* data)
{
    int client_fd = (long) data;
    char *buffer = (char*) malloc(BUFFER_SIZE * sizeof(char));

    while (true)
    {
        int amount_read = client_read_buffer(client_fd, buffer);
        if ( amount_read == -1 ) handle_error("ERROR: Unable to read.");
        // Quit
        if ( amount_read == 0 )
        {
            std::cout << "[" << (client_fd-3) << "] is leaving..." << std::endl;
            break;
        }
        if ( client_write_buffer(client_fd, buffer, amount_read) == - 1 ) handle_error("ERROR: Unable to write.");
        memset(buffer, 0, BUFFER_SIZE);
    }
    free(buffer);
    buffer = NULL;
    close(client_fd);
}

int set_nonblocking(int socketfd)
{
    int flags = fcntl(socketfd, F_GETFL, 0);
    if ( flags < 0 ) return -1;
    if ( fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) < 0 ) return -1;
    return 0;
}

int init_server_socket(void)
{
    int PORT = 8080;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( server_fd == -1 ) handle_error("Socket Failed.");
    if ( set_nonblocking(server_fd) == -1 ) handle_error("Failed to set server socket to non_blocking");

    int optval = 1;
    if ( setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval , sizeof(optval)) == - 1 ) handle_error("setsockopt Failed.");
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t server_address_len = sizeof(server_address);

    if ( bind(server_fd, (struct sockaddr *) &server_address, server_address_len) == -1 ) handle_error("Bind Failed.");
    if ( listen(server_fd, SOMAXCONN) == -1 ) handle_error("Listen failed.");

    std::cout << "Server initiated..." << std::endl;
    return server_fd;
}

int main(void)
{
    coroutine_init();
    // TODO: Allow user to inform PORT and IP by main args
    int server_fd = init_server_socket();
    while(true)
    {
        struct sockaddr_in client_address{};
        socklen_t client_address_len = sizeof(client_address);
        coroutine_sleep_read(server_fd);
        int client_fd = accept(server_fd, (struct sockaddr *) &client_address, &client_address_len );
        if ( client_fd < 0 ) handle_error("ERROR: Unable to accept new client");
        if ( set_nonblocking(client_fd) == -1 ) handle_error("Failed to set server socket to non_blocking.");

        coroutine_go(client_echo, (void *)(long int) client_fd);
    }

    close(server_fd);
    return 0;
}
