#pragma once
#include "connection_types.h"
#include "reactor.h"
#include "socket_utilities.h"
#include <functional>

#define CONN_BACKLOG 100000

namespace FiberConn {
class HttpServer {
private:
    int socket;
    int status;

public:
    FiberConn::IOReactor& ioc;
    HttpServer(IOReactor& reactor, char* address, char* port)
        : ioc(reactor)
        , status(0)
    {

        struct addrinfo* endpoint = FiberConn::getEndpoint(AF_INET, true, address, port);
        socket = FiberConn::getSocket(endpoint, true, false);
        FiberConn::bindAndListen(socket, endpoint, CONN_BACKLOG);
    }
    int listen(FiberConn::IOReactor& ioc, std::function<void(Connection*)> cb, std::function<void(Connection*)> err)
    {
        status = ioc.asyncAccept(socket, [&ioc, cb, err](struct epoll_event ev1) {
            /*Check for any disconnections*/
            if (ev1.events & EPOLLERR) {
                /*clean up*/
            } else if (ev1.events & EPOLLIN) {
                /*accept and register a new socket*/
                int newfd = FiberConn::acceptConnection(ev1.data.fd);
                if (newfd == -1) {
                    /*clean up*/
                } else {
                    /*create a new connection object*/
                    FiberConn::Connection* conn = new FiberConn::Clientconnection(newfd, ioc, cb, err);

                    uint32_t mask = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;

                    ioc.addTrack(newfd, mask, NEW_SOCK, [conn, cb, err](struct epoll_event ev2) {
                        conn->handleEvent(ev2, cb, err);
                    });
                }
            }
        });
        if (status == -1) {
            /*clean up*/
        } else {
            ioc.reactorRun();
        }
        return 0;
    }
};
}