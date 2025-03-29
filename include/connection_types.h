#pragma once
#include "reactor.h"
#include <unordered_map>
#include <functional>

namespace FiberConn{

    class Connection{
    protected:

        int socket;
        FiberConn::IOReactor &ioc;

    public:
        virtual void write(void *data, std::function<void(Connection *)>&) = 0;
        virtual void read(void *data) = 0;
        virtual void close() = 0;
        virtual void handleEvent(struct epoll_event evm, std::function<void (Connection *)>& cb) = 0;

        Connection(int sockfd, FiberConn::IOReactor &reactor)
        : socket(sockfd)
        , ioc(reactor)
        {
        }
        virtual ~Connection(){};
    };


    struct HttpRequest {
        std::string method;
        std::string URL;
        std::string version;
      
        std::string key, value;
        std::unordered_map<std::string, std::string> headers;
        std::vector<char> body;
        std::vector<char> buffer;
    };

    struct HttpResponse {
        std::string version;
        std::string statusCode;
        std::string statusMessage;
      
        std::string key, value;
        std::unordered_map<std::string, std::string> headers;
        std::vector<char> body;
        std::vector<char> buffer;
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
        void write(void *data, std::function<void(Connection *)> &cb){
            // store the data in response struct
            // then send the data via handle event method
        }
        void read(void *data){
            std::cout<<"data read complete\n";
        }
        void close(){
            FiberConn::closeConnection(socket);
        }
        void handleEvent(struct epoll_event ev, std::function<void (Connection *)>& cb){
            // all logic will be in handle event function
            if(ev.events & EPOLLIN){
                // read all the data   
                int bytes_read;
                while((bytes_read = recv(this->socket, buffer, sizeof(buffer), MSG_DONTWAIT)) > 0){
                    std::cout<<std::string(buffer, bytes_read)<<"\n";
                } 
                if(bytes_read == 0){
                    delete request;
                    delete response;
                    close();
                }
            }
            else if(ev.events & EPOLLOUT){

            }
            else{

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