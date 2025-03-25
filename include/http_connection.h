#pragma once

#include <http_parser.h>

namespace FiberConn
{
    class HttpConnection
    {
    private:
        bool isRequest;
        int socket_descriptor;

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
