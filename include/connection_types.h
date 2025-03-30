#pragma once
#include <functional>
#include <unordered_map>

#include "data_types.h"
#include "llhttp.h"
#include "reactor.h"

namespace FiberConn {

class Connection {
protected:
    int socket;
    FiberConn::IOReactor& ioc;

public:
    virtual void write(void* data,
        std::function<void(Connection*)>,
        std::function<void(Connection*)>)
        = 0;
    virtual void read(void* data) = 0;
    virtual void close() = 0;
    virtual void handleEvent(struct epoll_event evm,
        std::function<void(Connection*)>,
        std::function<void(Connection*)>)
        = 0;

    Connection(int sockfd, FiberConn::IOReactor& reactor)
        : socket(sockfd)
        , ioc(reactor)
    {
    }
    virtual ~Connection() { };
};

class Clientconnection : public Connection {
public:
    HttpRequest* request;
    HttpResponse* response;

    llhttp_t* parser;
    llhttp_settings_t* settings;

    size_t recv_buffer_size;
    std::vector<char> recvBuffer;

    size_t sent_bytes;
    std::vector<char> sendBuffer;

    std::function<void(Connection*)> cb;
    std::function<void(Connection*)> err;

    Clientconnection(int sockfd,
        FiberConn::IOReactor& reactor,
        std::function<void(Connection*)> success,
        std::function<void(Connection*)> error)
        : Connection(sockfd, reactor)
        , cb(success)
        , err(error)
    {
        request = new HttpRequest();
        response = nullptr;

        recv_buffer_size = 1024;
        recvBuffer = std::vector<char>(recv_buffer_size, 0);

        sent_bytes = 0;

        parser = new llhttp_t();
        settings = new llhttp_settings_t();

        llhttp_settings_init(settings);

        settings->on_message_begin = on_message_begin;
        settings->on_method = on_method;
        settings->on_url = on_url;
        settings->on_version = on_version;
        settings->on_header_field = on_header_field;
        settings->on_header_value = on_header_value;
        settings->on_headers_complete = on_headers_complete;
        settings->on_header_value_complete = on_header_value_complete;
        settings->on_body = on_body;
        settings->on_message_complete = on_message_complete;

        llhttp_init(parser, HTTP_REQUEST, settings);
        parser->data = this;
    }
    ~Clientconnection()
    {
        closeConnection(this->socket);
        delete request;
        delete parser;
        delete settings;
    }

    static int on_message_begin(llhttp_t* parser)
    {
        // std::cout << "[Callback] Message begin\n";
        return 0;
    }

    static int on_method(llhttp_t* parser, const char* at, size_t length)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->method.append(at, length);
        return 0;
    }

    static int on_url(llhttp_t* parser, const char* at, size_t length)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->URL.append(at, length);
        return 0;
    }

    static int on_version(llhttp_t* parser, const char* at, size_t length)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->version.append(at, length);
        return 0;
    }

    static int on_header_field(llhttp_t* parser, const char* at, size_t length)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->key.append(at, length);
        return 0;
    }

    static int on_header_value(llhttp_t* parser, const char* at, size_t length)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->value.append(at, length);
        return 0;
    }

    static int on_header_value_complete(llhttp_t* parser)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->headers[request->key] = request->value;
        request->key.clear();
        request->value.clear();
        return 0;
    }

    static int on_headers_complete(llhttp_t* parser) { return 0; }

    static int on_body(llhttp_t* parser, const char* at, size_t length)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        HttpRequest* request = conn->request;
        request->body.insert(request->body.end(), at, at + length);
        return 0;
    }

    static int on_message_complete(llhttp_t* parser)
    {
        Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
        conn->cb(conn);
        return 0;
    }

    void serialize()
    {
        if (!response)
            return;

        /*
        std::string version;
        std::string statusCode;
        std::string statusMessage;

        std::string key, value;
        std::unordered_map<std::string, std::string> headers;
        std::vector<char> body;
        std::vector<char> buffer;
        */

        if (!response)
            return;

        sendBuffer.clear(); 
        sendBuffer.reserve(1024 + response->body.size());

        // Append status line
        sendBuffer.insert(sendBuffer.end(), { 'H', 'T', 'T', 'P', '/' });
        sendBuffer.insert(sendBuffer.end(), response->version.begin(), response->version.end());
        sendBuffer.insert(sendBuffer.end(), {' '});
        sendBuffer.insert(sendBuffer.end(), response->statusCode.begin(), response->statusCode.end());
        sendBuffer.insert(sendBuffer.end(), {' '});
        sendBuffer.insert(sendBuffer.end(), response->statusMessage.begin(), response->statusMessage.end());
        sendBuffer.insert(sendBuffer.end(), { '\r', '\n' });

        // Append headers
        for (const auto& header : response->headers) {
            sendBuffer.insert(sendBuffer.end(), header.first.begin(), header.first.end());
            sendBuffer.insert(sendBuffer.end(), { ':', ' ' });
            sendBuffer.insert(sendBuffer.end(), header.second.begin(), header.second.end());
            sendBuffer.insert(sendBuffer.end(), { '\r', '\n' });
        }

        // End of headers
        sendBuffer.insert(sendBuffer.end(), { '\r', '\n' });

        // Append body
        sendBuffer.insert(sendBuffer.end(), response->body.begin(), response->body.end());
    }

    void write(void* data,
        std::function<void(Connection*)> success,
        std::function<void(Connection*)> error)
    {

        this->response = static_cast<HttpResponse*>(data);
        serialize();
        /*now the sendBuffer is filled*/
        uint32_t mask = EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
        FiberConn::Connection* conn = this;
        ioc.modifyTrack(this->socket, mask, HELPER_SOCK, [conn, success, error](struct epoll_event ev){
          conn->handleEvent(ev, success, error);
        });

    }
    void read(void* data_pointer)
    {
        *static_cast<HttpRequest**>(data_pointer) = this->request;
    }
    void close() { FiberConn::closeConnection(socket); }
    void handleEvent(struct epoll_event ev,
        std::function<void(Connection*)> success,
        std::function<void(Connection*)> error)
    {

        if (ev.events & EPOLLERR) {
            /*user will close the connection and delete its memory*/
            error(this);
        } else if (ev.events & EPOLLIN) {
            int read_bytes;
            while ((read_bytes = recv(this->socket, recvBuffer.data(),
                        recvBuffer.size(), MSG_DONTWAIT))
                > 0) {
                llhttp_errno_t llerror = llhttp_execute(this->parser, recvBuffer.data(), read_bytes);
                if (llerror != HPE_OK) {
                    std::cerr << "parsing error: " << llhttp_errno_name(llerror)
                              << " parser reason: " << parser->reason << "\n";
                    error(this);
                }
            }
            /* what happens if error */
            if (read_bytes == 0) {
                error(this);
                return;
            }
        } else if (ev.events & EPOLLOUT) {
          int bytes_sent;
          while((bytes_sent = send(this->socket, this->sendBuffer.data() + this->sent_bytes, this->sendBuffer.size()-this->sent_bytes, MSG_DONTWAIT)) > 0){
            this->sent_bytes += bytes_sent;

            if(this->sent_bytes >= this->sendBuffer.size()){
              this->sendBuffer.clear();
              this->sent_bytes = 0;
              /*Reset the connection*/
              success(this);
              break;
            }
          }

          if(bytes_sent == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
              return;
            }
            else{
              error(this);
              return;
            }
          }
        }
    }
};

class APIconnection : public Connection { };

class DBconnection : public Connection { };

class PostgreSQLconnection : public DBconnection { };

class SQLiteconnection : public DBconnection { };

} // namespace FiberConn