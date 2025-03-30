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
                     std::function<void(Connection*)>&,
                     std::function<void(Connection*)>&) = 0;
  virtual void read(void* data) = 0;
  virtual void close() = 0;
  virtual void handleEvent(struct epoll_event evm,
                           std::function<void(Connection*)>& cb,
                           std::function<void(Connection*)>& err) = 0;

  Connection(int sockfd, FiberConn::IOReactor& reactor)
      : socket(sockfd), ioc(reactor) {}
  virtual ~Connection() {};
};

class Clientconnection : public Connection {
 public:
  HttpRequest* request;
  HttpResponse* response;

  llhttp_t* parser;
  llhttp_settings_t* settings;

  char buffer[1024];

  std::function<void(Connection*)>& cb;
  std::function<void(Connection*)>& err;

  Clientconnection(int sockfd,
                   FiberConn::IOReactor& reactor,
                   std::function<void(Connection*)>& success,
                   std::function<void(Connection*)>& error)
      : Connection(sockfd, reactor), cb(success), err(error) {
    request = new HttpRequest();
    response = new HttpResponse();

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
  ~Clientconnection() {
    delete request;
    delete response;
    delete parser;
    delete settings;
  }

  static int on_message_begin(llhttp_t* parser) {
    std::cout << "[Callback] Message begin\n";
    return 0;
  }

  static int on_method(llhttp_t* parser, const char* at, size_t length) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->method.append(at, length);
    return 0;
  }

  static int on_url(llhttp_t* parser, const char* at, size_t length) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->URL.append(at, length);
    return 0;
  }

  static int on_version(llhttp_t* parser, const char* at, size_t length) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->version.append(at, length);
    return 0;
  }

  static int on_header_field(llhttp_t* parser, const char* at, size_t length) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->key.append(at, length);
    return 0;
  }

  static int on_header_value(llhttp_t* parser, const char* at, size_t length) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->value.append(at, length);
    return 0;
  }

  static int on_header_value_complete(llhttp_t* parser) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->headers[request->key] = request->value;
    request->key.clear();
    request->value.clear();
    return 0;
  }

  static int on_headers_complete(llhttp_t* parser) { return 0; }

  static int on_body(llhttp_t* parser, const char* at, size_t length) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    HttpRequest* request = conn->request;
    request->body.insert(request->body.end(), at, at + length);
    return 0;
  }

  static int on_message_complete(llhttp_t* parser) {
    Clientconnection* conn = static_cast<Clientconnection*>(parser->data);
    conn->cb(conn);
    return 0;
  }

  void write(void* data,
             std::function<void(Connection*)>& cb,
             std::function<void(Connection*)>& err) {}
  void read(void* data_pointer) {
    *static_cast<HttpRequest**>(data_pointer) = this->request;
  }
  void close() { FiberConn::closeConnection(socket); }
  void handleEvent(struct epoll_event ev,
                   std::function<void(Connection*)>& cb,
                   std::function<void(Connection*)>& err) {

    if (ev.events & EPOLLERR) {
      /*user will close the connection and delete its memory*/
      err(this);
    } else if (ev.events & EPOLLIN) {
      int read_bytes;
      while ((read_bytes = recv(this->socket, this->buffer,
                                sizeof(this->buffer), MSG_DONTWAIT)) > 0) {
        llhttp_errno_t error =
            llhttp_execute(this->parser, this->buffer, read_bytes);
        if (error != HPE_OK) {
          std::cerr << "parsing error: " << llhttp_errno_name(error)
                    << " parser reason: " << parser->reason << "\n";
          err(this);
        }
      }
      /* what happens if error */
      if(read_bytes == 0){
        closeConnection(this->socket);
      }
    } else if (ev.events & EPOLLOUT) {
    }
  }
};

class APIconnection : public Connection {};

class DBconnection : public Connection {};

class PostgreSQLconnection : public DBconnection {};

class SQLiteconnection : public DBconnection {};

}  // namespace FiberConn