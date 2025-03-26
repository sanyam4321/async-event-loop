#include "socket_utilities.h"
#include "reactor.h"
#include "http_connection.h"

std::unordered_map<int, FiberConn::HttpConnection> connection_state;

int main(int argc, char *argv[]){

    if(argc < 3){
        std::cerr<<"Too few arguments\n";
        return 1;
    }
    auto endpoint = FiberConn::getEndpoint(AF_INET, true, argv[1], argv[2]);
    int listenfd = FiberConn::getSocket(endpoint, true, false);
    FiberConn::bindAndListen(listenfd, endpoint, 1000);

    FiberConn::IOReactor ioc(1000);
    ioc.asyncAccept(listenfd, [&ioc](struct epoll_event ev){
        int newfd = FiberConn::acceptConnection(ev.data.fd);
        uint32_t mask = EPOLLIN | EPOLLET | EPOLLRDHUP| EPOLLERR;
        ioc.addTrack(newfd, mask, FiberConn::NEW_SOCK, [&ioc](struct epoll_event ev){
            if(ev.events & EPOLLRDHUP){
                std::cout<<"socket disconnected by the client\n";
                return;
            }
            int bytes_read;
            char temp_buffer[1024];
            while((bytes_read = recv(ev.data.fd, temp_buffer, 1024, MSG_DONTWAIT)) > 0){
                std::cout<<std::string(temp_buffer, bytes_read)<<"\n";
            }
            if(bytes_read == 0){
                close(ev.data.fd);
                return;
            }
            char api_ip[] = "127.0.0.1";
            char api_port[] = "3000";
            int api_sock = FiberConn::createConnection(AF_INET, api_ip, api_port);
            std::cout<<"api socket: "<<api_sock<<"\n";
            uint32_t mask = EPOLLOUT | EPOLLRDHUP| EPOLLERR;
            ioc.addTrack(api_sock, mask, FiberConn::HELPER_SOCK, [&ioc](struct epoll_event ev){
                int send_bytes = 0;
                char request_buffer[] = 
                    "GET / HTTP/1.1"
                ;
                while((send_bytes = send(ev.data.fd, request_buffer, sizeof(request_buffer), MSG_DONTWAIT)) < sizeof(request_buffer)){
                    std::cout<<"sent: "<<send_bytes<<"\n";
                }
                if(send_bytes != 0){
                    ioc.modifyTrack(ev.data.fd, EPOLLIN | EPOLLET, FiberConn::HELPER_SOCK, [&ioc](struct epoll_event ev){
                        int bytes_read;
                        char temp_buffer[1024];
                        while((bytes_read = recv(ev.data.fd, temp_buffer, 1024, MSG_DONTWAIT)) > 0){
                            std::cout<<std::string(temp_buffer, bytes_read)<<"\n";
                        }
                    });
                }
            });
        });

    });
    ioc.reactorRun();
    return 0;
}