#pragma once
#include "socket_utilities.h"
#include "reactor.h"
#include "functional"
#include "connection_types.h"

#define CONN_BACKLOG 10000

namespace FiberConn {
    class HttpServer{
        private:

        int socket;
        int status;
        
        public:

        FiberConn::IOReactor &ioc;
        HttpServer(IOReactor &reactor, char *address, char *port)
        : ioc(reactor)
        , status(0)   
        {
            
            struct addrinfo *endpoint = FiberConn::getEndpoint(AF_INET, true, address, port);
            socket = FiberConn::getSocket(endpoint, true, false);
            FiberConn::bindAndListen(socket, endpoint, CONN_BACKLOG);
        }
        int listen(std::function<void(Connection *)> cb){
            status = ioc.asyncAccept(socket, [this, &cb](struct epoll_event ev1){
                /*Check for any disconnections*/
                if(ev1.events & EPOLLERR || ev1.events & EPOLLHUP){
                    /*clean up*/
                }
                else if(ev1.events & EPOLLIN){
                    /*accept and register a new socket*/
                    int newfd = FiberConn::acceptConnection(ev1.data.fd);
                    if(newfd == -1){
                        /*clean up*/
                    }
                    else{
                        /*create a new connection object*/
                        FiberConn::Connection *conn = new FiberConn::Clientconnection(newfd, this->ioc);
                        uint32_t mask = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
                        ioc.addTrack(newfd, mask, NEW_SOCK, [conn, &cb](struct epoll_event ev2){
                            if(ev2.events & EPOLLERR || ev2.events & EPOLLHUP || ev2.events & EPOLLRDHUP){
                                std::cout<<"connection closed\n";
                                FiberConn::closeConnection(ev2.data.fd);
                                delete conn;
                            }   
                            else{
                                conn->handleEvent(ev2, cb);
                            }
                        });
                    }
                }

            });
            if(status == -1){
                /*clean up*/
            }
            else{
                ioc.reactorRun();
            }
            return 0;
        }
    };
}