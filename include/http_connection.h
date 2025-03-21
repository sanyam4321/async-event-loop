#pragma once

#include <arpa/inet.h>
#include <http_parser.h>

namespace FiberConn
{
    class HttpConnection
    {
    private:
        int socket_descriptor;

        char ip_address[INET6_ADDRSTRLEN];
        int port_address;

    public:
        FiberConn::HttpRequest *req;
        FiberConn::HttpResponse *res;

        HttpConnection(int id)
        {
            socket_descriptor = id;
            req = new HttpRequest(id);
            res = new HttpResponse(id);
        }
        ~HttpConnection()
        {
            delete req;
            delete res;
            std::cout<<"Connection Data Deleted!\n";
        }
    };
}
