#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

#include <unordered_map>
#include <vector>

#include <http_connection.h>

#define CONN_PORT "3490"
#define CONN_BACKLOG 100
#define MAX_EVENTS 100
#define BUFFER_SIZE 100


int handleNewConnection(int sockfd, int epollfd, struct epoll_event &ev_hint, std::unordered_map<int, FiberConn::HttpConnection *> &fd_map)
{
    int newfd;
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);

    if ((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size)) == -1)
    {
        std::cerr << "accept error: " << strerror(errno) << std::endl;
        return -1;
    }

    if (fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "socket non-blocking error: " << strerror(errno) << std::endl;
        exit(1);
    }

    char client_ip[INET6_ADDRSTRLEN];
    int client_port;

    if (client_addr.ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &ipv4->sin_addr, client_ip, sizeof(client_ip));
        client_port = ntohs(ipv4->sin_port);
    }
    else if (client_addr.ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&client_addr;
        inet_ntop(AF_INET6, &ipv6->sin6_addr, client_ip, sizeof(client_ip));
        client_port = ntohs(ipv6->sin6_port);
    }
    else
    {
        std::cerr << "Unknown address family" << std::endl;
        close(newfd);
    }

    std::cout << "client ip: " << client_ip << " client port: " << client_port << std::endl;

    ev_hint.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    ev_hint.data.fd = newfd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newfd, &ev_hint) == -1)
    {
        std::cerr << "epoll ctl error: " << strerror(errno) << std::endl;
    }

    FiberConn::HttpConnection *new_connection = new FiberConn::HttpConnection(newfd);

    fd_map.insert({newfd, new_connection});

    return 0;
}

int handleAlreadyConnected(int sockfd, struct epoll_event &client_event, std::unordered_map<int, FiberConn::HttpConnection *> &fd_map)
{

    int clientfd = client_event.data.fd;
    uint32_t events_mask = client_event.events;
    FiberConn::HttpConnection *old_connection = fd_map[clientfd];

    if (events_mask & EPOLLHUP || events_mask & EPOLLERR || events_mask & EPOLLRDHUP)
    {
        // disconnect the client and clear its associated data structures
        delete old_connection;
        std::cout << "client: " << clientfd << " disconnected --> EPOLL" << std::endl;
        close(clientfd);
    }

    else if (events_mask & EPOLLIN)
    {
        int bytes_received = 0;
        char buffer[BUFFER_SIZE];

        while ((bytes_received = recv(clientfd, buffer, sizeof(buffer), 0)) > 0)
        {
            // append to http buffer
            old_connection->req->appendToBuffer(buffer, bytes_received);
            std::cout << std::string(buffer, bytes_received) << std::endl;
        }

        if (bytes_received == 0)
        {
            // disconnect the client and clear its associated data structures
            delete old_connection;
            std::cout << "client: " << clientfd << " disconnected --> RECV" << std::endl;
            close(clientfd);
        }
        else if (bytes_received == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "client: " << clientfd << " no more data to read" << std::endl;
            }
            if(old_connection->req->request_complete){
                old_connection->req->printRequest();
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int status;
    struct addrinfo hints;
    struct addrinfo *servlist = nullptr; // linked list to all addresses

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, CONN_PORT, &hints, &servlist)) != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        exit(1);
    }

    struct addrinfo *server_info;

    for (server_info = servlist; server_info != nullptr; server_info = server_info->ai_next)
    {
        char ip_string[INET6_ADDRSTRLEN];
        void *addr;
        int port;
        std::string ip_version;

        if (server_info->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)server_info->ai_addr;
            addr = &(ipv4->sin_addr);
            port = ntohs(ipv4->sin_port);
            ip_version = "IPV4";

            // convert addr to readable format
            inet_ntop(server_info->ai_family, addr, ip_string, sizeof(ip_string));
            std::cout << ip_string << " " << port << " " << ip_version << std::endl;
            break;
        }
        else
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)server_info->ai_addr;
            addr = &(ipv6->sin6_addr);
            port = ntohs(ipv6->sin6_port);
            ip_version = "IPV6";

            // convert addr to readable format
            inet_ntop(server_info->ai_family, addr, ip_string, sizeof(ip_string));
            std::cout << ip_string << " " << port << " " << ip_version << std::endl;
        }
    }

    int sockfd;
    if ((sockfd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) == -1)
    {
        std::cerr << "socket error: " << strerror(errno) << std::endl;
        exit(1);
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        std::cerr << "socket reuse error: " << "setsockopt" << std::endl;
        exit(1);
    }

    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "socket non-blocking error: " << strerror(errno) << std::endl;
        exit(1);
    }

    if (bind(sockfd, server_info->ai_addr, server_info->ai_addrlen) == -1)
    {
        std::cerr << "bind error: " << strerror(errno) << std::endl;
        exit(1);
    }

    if (listen(sockfd, CONN_BACKLOG) == -1)
    {
        std::cerr << "listen error: " << strerror(errno) << std::endl;
        exit(1);
    }

    std::cout << "server listening\n";

    struct epoll_event ev_hint, events[MAX_EVENTS];
    int epollfd, num_events = 0;

    if ((epollfd = epoll_create1(0)) == -1)
    {
        std::cerr << "epoll creare error: " << strerror(errno) << std::endl;
        exit(1);
    }

    ev_hint.events = EPOLLIN;
    ev_hint.data.fd = sockfd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev_hint) == -1)
    {
        std::cerr << "epoll ctl error: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::unordered_map<int, FiberConn::HttpConnection *> fd_map;

    while (true)
    {
        num_events = epoll_wait(epollfd, events, MAX_EVENTS, 5);
        if (num_events == -1)
        {
            std::cerr << "epoll wait error: " << strerror(errno) << std::endl;
            exit(1);
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == sockfd)
            {
                // handle new connection
                handleNewConnection(sockfd, epollfd, ev_hint, fd_map);
            }
            else
            {
                // handle existing connection
                handleAlreadyConnected(sockfd, events[i], fd_map);
            }
        }
    }

    freeaddrinfo(servlist);

    return 0;
}