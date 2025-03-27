#pragma once
#include "reactor.h"

namespace FiberConn{

    class Connection{
    private:

        int socket;
        FiberConn::IOReactor &ioc;

    public:

        Connection(int sockfd, FiberConn::IOReactor &reactor)
        : socket(sockfd)
        , ioc(reactor)
        {

        }
    };

    class HTTPConnection {
    public:
        
        void serialize();
        void deserialize();
        

    };

    class DBConnection {

    };

}