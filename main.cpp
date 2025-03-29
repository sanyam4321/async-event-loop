#include "llhttp.h"
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

// Structure to store parsed HTTP request data
struct HttpRequest {
  std::string method;
  std::string URL;
  std::string version;

  std::string key, value;
  std::unordered_map<std::string, std::string> headers;
  std::vector<char> body;
  std::vector<char> buffer;
};

// Callback: called at the start of a message
int on_message_begin(llhttp_t *parser) {
  std::cout << "[Callback] Message begin\n";
  return 0;
}

// Callback: called when the method is parsed
int on_method(llhttp_t *parser, const char *at, size_t length) {
  std::cout << "5\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->method.append(at, length);
  return 0;
}

// Callback: called when the URL is parsed
int on_url(llhttp_t *parser, const char *at, size_t length) {
  std::cout << "6\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->URL.append(at, length);
  return 0;
}

// Callback: called when the HTTP version is parsed
int on_version(llhttp_t *parser, const char *at, size_t length) {
  std::cout << "7\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->version.append(at, length);
  return 0;
}

// Callback: called when a header field is parsed
int on_header_field(llhttp_t *parser, const char *at, size_t length) {
  std::cout << "8\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->key.append(at, length);
  return 0;
}

// Callback: called when a header value is parsed
int on_header_value(llhttp_t *parser, const char *at, size_t length) {
  std::cout << "9\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->value.append(at, length);
  std::cout << request->value << "\n";
  return 0;
}

int on_header_value_complete(llhttp_t *parser) {
  std::cout << "header complete\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->headers[request->key] = request->value;
  request->key.clear();
  request->value.clear();
  return 0;
}

// Callback: called once all headers have been parsed
int on_headers_complete(llhttp_t *parser) {
  std::cout << "[Callback] Headers complete\n";
  return 0;
}

// Callback: called when the body is parsed
int on_body(llhttp_t *parser, const char *at, size_t length) {
  std::cout << "10\n";
  HttpRequest *request = static_cast<HttpRequest *>(parser->data);
  request->body.insert(request->body.end(), at, at + length);
  return 0;
}

// Callback: called when the message is fully parsed
int on_message_complete(llhttp_t *parser) {
  std::cout << "[Callback] Message complete\n";
  return 0;
}

// Function to print the parsed HTTP request
void printHttpRequest(const HttpRequest &request) {
  std::cout << "HTTP Request:" << std::endl;
  std::cout << "  Method:  " << request.method << std::endl;
  std::cout << "  URL:     " << request.URL << std::endl;
  std::cout << "  Version: " << request.version << std::endl;
  std::cout << "  Headers:" << std::endl;
  for (const auto &header : request.headers) {
    std::cout << "    " << header.first << ": " << header.second << std::endl;
  }
  std::cout << "  Body:" << std::endl;
  if (!request.body.empty()) {
    std::string bodyString(request.body.begin(), request.body.end());
    std::cout << bodyString << std::endl;
  } else {
    std::cout << "    [empty]" << std::endl;
  }
}

int main() {
  // 1. Create parser and settings
  llhttp_t parser;
  llhttp_settings_t settings;

  // 2. Initialize the settings and set callback pointers
  llhttp_settings_init(&settings);

  std::cout << "1\n";
  settings.on_message_begin = on_message_begin;
  settings.on_method = on_method;
  settings.on_url = on_url;
  settings.on_version = on_version;
  settings.on_header_field = on_header_field;
  settings.on_header_value = on_header_value;
  settings.on_headers_complete = on_headers_complete;
  settings.on_header_value_complete = on_header_value_complete;
  settings.on_body = on_body;
  settings.on_message_complete = on_message_complete;
  std::cout << "2\n";
  // 3. Initialize the parser for HTTP requests
  llhttp_init(&parser, HTTP_REQUEST, &settings);
  HttpRequest *request = new HttpRequest();
  parser.data = request;
  std::cout << "3\n";
  // 4. Define sample HTTP request data (using chunked encoding)
  const char *chunks[] = {
      "POST /upl",
      "oad HTTP/1.",
      "1\r\nHost: ex",
      "ample.com\r\nTransfer-Encoding: chu",
      "nked\r\n\r\n5\r\nHello\r\n",          // Chunk size 5 (Hello)
      "16\r\n, world! This is a tes\r\n",    // Chunk size 1A (26 bytes)
      "11\r\n t message to sim\r\n0\r\n\r\n" // End of chunked message
  };

  // Simulate receiving and parsing data in multiple parts
  for (const char *chunk : chunks) {
    llhttp_errno_t err = llhttp_execute(&parser, chunk, std::strlen(chunk));
    if (err != HPE_OK) {
      std::cerr << "Parsing error: " << llhttp_errno_name(err) << " - "
                << parser.reason << "\n";
      delete request;
      return 1;
    }
  }

  // Print final parsed request
  printHttpRequest(*request);

  // Clean up
  delete request;
  return 0;
}