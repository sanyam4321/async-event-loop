#pragma once
#include "reactor.h"
#include <unordered_map>
#include <functional>
#include "data_types.h"

namespace FiberConn{

    class Connection{
    protected:

        int socket;
        FiberConn::IOReactor &ioc;

    public:
        virtual void write(void *data, std::function<void(Connection *)>&, std::function<void(Connection *)> &) = 0;
        virtual void read(void *data) = 0;
        virtual void close() = 0;
        virtual void handleEvent(struct epoll_event evm, std::function<void (Connection *)>& cb, std::function<void (Connection *)>& err) = 0;

        Connection(int sockfd, FiberConn::IOReactor &reactor)
        : socket(sockfd)
        , ioc(reactor)
        {
        }
        virtual ~Connection(){};
    };


   

    class Clientconnection : public Connection{

        public:

        HttpRequest *request;
        HttpResponse *response;

        char buffer[1024];

        Clientconnection(int sockfd, FiberConn::IOReactor &reactor)
        :Connection(sockfd, reactor)
        {
            request = new HttpRequest();
            response = new HttpResponse();
        }
        ~Clientconnection(){
            delete request;
            delete response;
        }
        void write(void *data, std::function<void(Connection *)> &cb, std::function<void(Connection *)> &err){
            // store the data in response struct
            // then send the data via handle event method
        }
        void read(void *data){
            std::cout<<"data read complete\n";
        }
        void close(){
            FiberConn::closeConnection(socket);
        }
        void handleEvent(struct epoll_event ev, std::function<void (Connection *)>& cb, std::function<void (Connection *)>& err){
            
            if(ev.events & EPOLLERR){
                err(this);
            }
            else if(ev.events & EPOLLIN){
                
            }
            else if(ev.events & EPOLLOUT){

            }
        }
    };

    class APIconnection : public Connection {

    };

    class DBconnection : public Connection {

    };

    class PostgreSQLconnection : public DBconnection {

    };

    class SQLiteconnection : public DBconnection {

    };

}