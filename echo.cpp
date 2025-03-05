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

void trim(char *buffer)
{
    while (*buffer && isspace(*buffer)) buffer++;

    char *end = buffer + strlen(buffer) - 1;
    while (end > buffer && isspace(*end)) end--;

    *(end + 1) = '\0';
}

struct Client
{
    int fd;
};
std::vector<Client> clients;

struct sockaddr_in address;
socklen_t address_len = sizeof(address);

int init_server_socket(void)
{
    int PORT = 8080;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( sockfd == -1 ) handle_error("Socket Failed.");
    if ( fcntl(sockfd, F_SETFL, O_NONBLOCK ) ) handle_error("Failed to set socket to non_blocking.");

    int optval = 1;
    if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval , sizeof(optval)) == - 1 ) handle_error("setsockopt Failed.");

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if ( bind(sockfd, (struct sockaddr *) &address, sizeof(address)) == -1 ) handle_error("Bind Failed.");
    if ( listen(sockfd, SOMAXCONN) == -1 ) handle_error("Listen failed.");

    std::cout << "Server initiated..." << std::endl;
    return sockfd;
}

int main(void)
{
    int sockfd = init_server_socket();

    bool shutdown = false;
    char buffer[BUFFER_SIZE];
    while(!shutdown)
    {
        int clientfd;
        if ( (clientfd = accept(sockfd, (struct sockaddr *) &address, &address_len )) != -1 )
        {
            clients.push_back(Client{
                .fd = clientfd,
            });
            if ( fcntl(clientfd, F_SETFL, O_NONBLOCK ) ) handle_error("Failed to set client to non_blocking.");
            std::cout << "New connection: " << clientfd << " ~ (Total: " << clients.size() << ")" << std::endl;
        }

        for ( size_t i = 0; i < clients.size(); i++ )
        {
            memset(&buffer, 0, BUFFER_SIZE);
            ssize_t amount_read = read(clients[i].fd, buffer, BUFFER_SIZE);

            if ( amount_read > 0 )
            {
                buffer[amount_read] = '\0';
                // trim(buffer);
                while ( ( amount_read > 0 ) && ( buffer[amount_read - 1] == '\n' || buffer[amount_read - 1] == '\r' ) )
                {
                    buffer[--amount_read] = '\0';
                }

                std::string check (buffer);
                if (check == "quit")
                {
                    std::cout << "[" << i << "] is leaving..." << std::endl;
                    close(clients[i].fd);
                    clients.erase(clients.begin() + i);
                    // Removing a Client implies in shifting Clients in "std::vector<Client>",
                    // by decrementing i "(i--)", we ensure that the newly shifted Client at "i" gets checked.
                    // Which wouldn't happen since for loop increments i "(i++)"
                    i--;
                }
                else if ( check == "shutdown" )
                {
                    std::cout << "[" << i << "] closed the server..." << std::endl;
                    for ( size_t j = 0; j < clients.size(); j++ ) close(clients[j].fd);
                    shutdown = true;
                    close(sockfd);
                } else
                {
                    std::cout << "[" << i << "] SAID: " << buffer << " (BYTES: " << amount_read << ")" << std::endl;
                    ssize_t amount_written = 0;
                    // TODO: Change buffer to make it more "readable" at "write" operation
                    while ( amount_written != amount_read ) amount_written += write(clients[i].fd, buffer, (amount_read - amount_written));
                }



            }
        }

    }



    return 0;
}
