#pragma once
#include <functional>
#include <unordered_map>

#include "data_types.h"
#include "reactor.h"
#include "llhttp.h"

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

  llhttp_t *parser;
  llhttp_settings_t *settings;

  char buffer[1024];

  Clientconnection(int sockfd, FiberConn::IOReactor& reactor)
      : Connection(sockfd, reactor) {
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
  

  static int on_message_begin(llhttp_t *parser) {
    std::cout << "[Callback] Message begin\n";
    return 0;
  }
  
  static int on_method(llhttp_t *parser, const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->method.append(at, length);
    return 0;
  }
  
  static int on_url(llhttp_t *parser, const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->URL.append(at, length);
    return 0;
  }
  
  static int on_version(llhttp_t *parser, const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->version.append(at, length);
    return 0;
  }
  
  static int on_header_field(llhttp_t *parser, const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->key.append(at, length);
    return 0;
  }
  
  static int on_header_value(llhttp_t *parser, const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->value.append(at, length);
    return 0;
  }
  
  static int on_header_value_complete(llhttp_t *parser) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->headers[request->key] = request->value;
    request->key.clear();
    request->value.clear();
    return 0;
  }
  
  static int on_headers_complete(llhttp_t *parser) {
    return 0;
  }
  
  static int on_body(llhttp_t *parser, const char *at, size_t length) {
    HttpRequest *request = static_cast<HttpRequest *>(parser->data);
    request->body.insert(request->body.end(), at, at + length);
    return 0;
  }
  
  static int on_message_complete(llhttp_t *parser) {
    return 0;
  }


  void write(void* data,
             std::function<void(Connection*)>& cb,
             std::function<void(Connection*)>& err) {}
  void read(void* data) {}
  void close() { FiberConn::closeConnection(socket); }
  void handleEvent(struct epoll_event ev,
                   std::function<void(Connection*)>& cb,
                   std::function<void(Connection*)>& err) {
    if (ev.events & EPOLLERR) {
      /*user will close the connection and delete its memory*/
      err(this);
    } else if (ev.events & EPOLLIN) {

    } else if (ev.events & EPOLLOUT) {

    }
  }
};

class APIconnection : public Connection {};

class DBconnection : public Connection {};

class PostgreSQLconnection : public DBconnection {};

class SQLiteconnection : public DBconnection {};

}  // namespace FiberConn