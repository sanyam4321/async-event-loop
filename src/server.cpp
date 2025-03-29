#include "socket_utilities.h"
#include "reactor.h"
#include "listening_server.h"
#include "connection_types.h"

int main(int argc, char *argv[]){

    if(argc < 3){
        std::cerr<<"Too few arguments\n";
        return 1;
    }
    FiberConn::IOReactor ioc(1000);
    FiberConn::HttpServer server(ioc, argv[1], argv[2]);

    server.listen([](FiberConn::Connection *client){
        client->read(NULL);
    });

    return 0;
}