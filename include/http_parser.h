#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <exception>

namespace FiberConn
{
    enum headers
    {
        DEFAULT,
        HOST,
        CONTENT_TYPE,
        TRANSFER_ENCODING,
        CONTENT_LENGTH,
        USER_AGENT,
        ACCEPT,
        ACCEPT_ENCODING,
        ACCEPT_LANGUAGE,
        AUTHORIZATION,
        CACHE_CONTROL,
        CONNECTION,
        COOKIE,
        DATE,
        EXPECT,
        FROM,
        IF_MATCH,
        IF_MODIFIED_SINCE,
        IF_NONE_MATCH,
        IF_RANGE,
        IF_UNMODIFIED_SINCE,
        MAX_FORWARDS,
        ORIGIN,
        PRAGMA,
        PROXY_AUTHORIZATION,
        RANGE,
        REFERER,
        TE,
        UPGRADE,
        VIA,
        WARNING
    };

    class HttpRequest
    {
    public:
        size_t req_id;
        size_t i;
        bool request_line_ok;
        bool headers_ok;
        bool chunked_transfer;
        bool request_complete;

        bool r1;
        bool n1;
        bool r2;
        bool n2;

        static std::unordered_map<std::string, enum headers> text_header;

        std::vector<std::string> request_line;
        std::unordered_map<enum headers, std::string> header_value;
        std::vector<char> body;
        size_t length;

        std::string temp;
        enum headers current_header;

        std::vector<char> request_buffer;

        int parseRequest()
        {
            size_t size = request_buffer.size();

            for (; i < size; i++)
            {
                if (!request_line_ok)
                {
                    /*Parsing request line*/

                    if (request_buffer[i] == ' ' && !temp.empty())
                    {
                        request_line.push_back(temp);
                        temp.clear();
                    }
                    else if (request_buffer[i] == '\r')
                    {
                        r1 = true;
                    }
                    else if (request_buffer[i] == '\n')
                    {
                        n1 = true;
                    }
                    else
                    {
                        temp.push_back(request_buffer[i]);
                    }

                    if (r1 && n1)
                    {
                        /*End of request line script*/
                        request_line.push_back(temp);
                        temp.clear();
                        r1 = false;
                        n1 = false;
                        request_line_ok = true;

                        if (request_line.size() != 3)
                        {
                            /*Invalid request line format*/
                            return -1;
                        }

                        std::cout << "REQUEST LINE COMPLETE\n";
                    }
                }
                else if (!headers_ok)
                {
                    /*Parsing headers*/

                    if (request_buffer[i] == ':')
                    {
                        if (text_header.count(temp) == 0)
                        {
                            std::cerr << "Warning: Unknown header '" << temp << "'\n";
                            current_header = DEFAULT;
                        }
                        else
                        {
                            current_header = text_header[temp];
                        }
                        temp.clear();
                    }
                    else if (request_buffer[i] == '\r')
                    {
                        if (!r1)
                        {
                            r1 = true;
                        }
                        else
                        {
                            r2 = true;
                        }
                    }
                    else if (request_buffer[i] == '\n')
                    {
                        if (!n1)
                        {
                            n1 = true;
                        }
                        else
                        {
                            n2 = true;
                        }
                    }
                    else
                    {
                        r1 = false;
                        n1 = false;
                        temp.push_back(request_buffer[i]);
                    }

                    if (r2 && n2)
                    {
                        if (header_value.empty())
                        {
                            return -1;
                        }
                        /*Header parsing complete*/
                        headers_ok = true;
                        r1 = false;
                        r2 = false;
                        n1 = false;
                        n2 = false;
                        if (header_value.count(TRANSFER_ENCODING) > 0 && header_value[TRANSFER_ENCODING] == "chunked")
                        {
                            chunked_transfer = true;
                        }
                        else
                        {
                            try
                            {
                                length = std::stoi(header_value[CONTENT_LENGTH]);
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << "Error: Invalid Content-Length value\n"
                                     << header_value[CONTENT_LENGTH];
                                return -1;
                            }
                        }
                    }
                    else if (r1 && n1)
                    {
                        /*Header value read*/
                        if (current_header != DEFAULT)
                        {
                            header_value[current_header] = temp;
                        }
                        temp.clear();
                        current_header = DEFAULT;
                    }
                }
                else
                {
                    /*Parsing body*/
                    if (chunked_transfer)
                    {
                    }
                    else
                    {
                        std::cout << request_buffer[i] << "-->" << length << std::endl;
                        body.push_back(request_buffer[i]);
                        length--;
                        if (length == 0)
                        {
                            request_complete = true;
                        }
                    }
                }
            }
            if (request_complete)
            {
                return 1;
            }
            return 0;
        }

        

        HttpRequest(size_t id)
            : req_id(id),
              i(0),
              request_line_ok(false),
              headers_ok(false),
              chunked_transfer(false),
              request_complete(false),
              r1(false),
              n1(false),
              r2(false),
              n2(false),
              length(0)

        {
        }


        int requestToBuffer(){

        }

        int appendToBuffer(char read_buffer[], size_t bytes_read)
        {
            request_buffer.insert(request_buffer.end(), read_buffer, read_buffer + bytes_read);
            return parseRequest();
        }
        void printRequest()
        {
            std::cout << "\n----- HTTP Request Parsed -----\n";
            std::cout << "Method: " << request_line[0] << "\n";
            std::cout << "Path: " << request_line[1] << "\n";
            std::cout << "Version: " << request_line[2] << "\n";
            std::cout << "Headers:\n";
            for (const auto &h : header_value)
            {
                std::cout << "  " << h.first << ": " << h.second << "\n";
            }
            std::cout << "Body size: " << body.size() << std::endl;
            std::cout << "Body: " << std::string(body.begin(), body.end()) << "\n";
            std::cout << "--------------------------------\n";
        }

    };

    class HttpResponse
    {
    public:
        size_t res_id;
        size_t i;
        bool status_line_ok;
        bool headers_ok;
        bool chunked_transfer;
        bool response_complete;

        bool r1;
        bool n1;
        bool r2;
        bool n2;

        static std::unordered_map<std::string, enum headers> text_header;

        std::vector<std::string> status_line;
        std::unordered_map<enum headers, std::string> header_value;
        std::vector<char> body;
        size_t length;

        std::string temp;
        enum headers current_header;

        std::vector<char> response_buffer;

        int parseResponse()
        {
            size_t size = response_buffer.size();

            for (; i < size; i++)
            {
                if (!status_line_ok)
                {
                    /*Parsing request line*/

                    if (response_buffer[i] == ' ' && !temp.empty())
                    {
                        status_line.push_back(temp);
                        temp.clear();
                    }
                    else if (response_buffer[i] == '\r')
                    {
                        r1 = true;
                    }
                    else if (response_buffer[i] == '\n')
                    {
                        n1 = true;
                    }
                    else
                    {
                        temp.push_back(response_buffer[i]);
                    }

                    if (r1 && n1)
                    {
                        /*End of request line script*/
                        status_line.push_back(temp);
                        temp.clear();
                        r1 = false;
                        n1 = false;
                        status_line_ok = true;

                        if (status_line.size() != 3)
                        {
                            /*Invalid request line format*/
                            return -1;
                        }

                        std::cout << "STATUS LINE COMPLETE\n";
                    }
                }
                else if (!headers_ok)
                {
                    /*Parsing headers*/

                    if (response_buffer[i] == ':')
                    {
                        if (text_header.count(temp) == 0)
                        {
                            std::cerr << "Warning: Unknown header '" << temp << "'\n";
                            current_header = DEFAULT;
                        }
                        else
                        {
                            current_header = text_header[temp];
                        }
                        temp.clear();
                    }
                    else if (response_buffer[i] == '\r')
                    {
                        if (!r1)
                        {
                            r1 = true;
                        }
                        else
                        {
                            r2 = true;
                        }
                    }
                    else if (response_buffer[i] == '\n')
                    {
                        if (!n1)
                        {
                            n1 = true;
                        }
                        else
                        {
                            n2 = true;
                        }
                    }
                    else
                    {
                        r1 = false;
                        n1 = false;
                        temp.push_back(response_buffer[i]);
                    }

                    if (r2 && n2)
                    {
                        if (header_value.empty())
                        {
                            return -1;
                        }
                        /*Header parsing complete*/
                        headers_ok = true;
                        r1 = false;
                        r2 = false;
                        n1 = false;
                        n2 = false;
                        if (header_value.count(TRANSFER_ENCODING) > 0 && header_value[TRANSFER_ENCODING] == "chunked")
                        {
                            chunked_transfer = true;
                        }
                        else
                        {
                            try
                            {
                                length = std::stoi(header_value[CONTENT_LENGTH]);
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << "Error: Invalid Content-Length value\n"
                                     << header_value[CONTENT_LENGTH];
                                return -1;
                            }
                        }
                    }
                    else if (r1 && n1)
                    {
                        /*Header value read*/
                        if (current_header != DEFAULT)
                        {
                            header_value[current_header] = temp;
                        }
                        temp.clear();
                        current_header = DEFAULT;
                    }
                }
                else
                {
                    /*Parsing body*/
                    if (chunked_transfer)
                    {
                    }
                    else
                    {
                        std::cout << response_buffer[i] << "-->" << length << std::endl;
                        body.push_back(response_buffer[i]);
                        length--;
                        if (length == 0)
                        {
                            response_complete = true;
                        }
                    }
                }
            }
            if (response_complete)
            {
                return 1;
            }
            return 0;
        }

        HttpResponse(size_t id)
            : res_id(id),
              i(0),
              status_line_ok(false),
              headers_ok(false),
              chunked_transfer(false),
              response_complete(false),
              r1(false),
              n1(false),
              r2(false),
              n2(false),
              length(0)

        {
        }


        int responseToBuffer(){

        }

        int appendToBuffer(char read_buffer[], size_t bytes_read)
        {
            response_buffer.insert(response_buffer.end(), read_buffer, read_buffer + bytes_read);
            return parseResponse();
        }

        
        void printResponse()
        {
            std::cout << "\n----- HTTP Response Parsed -----\n";
            std::cout << "Method: " << status_line[0] << "\n";
            std::cout << "Path: " << status_line[1] << "\n";
            std::cout << "Version: " << status_line[2] << "\n";
            std::cout << "Headers:\n";
            for (const auto &h : header_value)
            {
                std::cout << "  " << h.first << ": " << h.second << "\n";
            }
            std::cout << "Body size: " << body.size() << std::endl;
            std::cout << "Body: " << std::string(body.begin(), body.end()) << "\n";
            std::cout << "--------------------------------\n";
        }

    };









    std::unordered_map<std::string, headers> HttpRequest::text_header = {
        {"", DEFAULT},
        {"Host", HOST},
        {"Content-Type", CONTENT_TYPE},
        {"Transfer-Encoding", TRANSFER_ENCODING},
        {"Content-Length", CONTENT_LENGTH},
        {"User-Agent", USER_AGENT},
        {"Accept", ACCEPT},
        {"Accept-Encoding", ACCEPT_ENCODING},
        {"Accept-Language", ACCEPT_LANGUAGE},
        {"Authorization", AUTHORIZATION},
        {"Cache-Control", CACHE_CONTROL},
        {"Connection", CONNECTION},
        {"Cookie", COOKIE},
        {"Date", DATE},
        {"Expect", EXPECT},
        {"From", FROM},
        {"If-Match", IF_MATCH},
        {"If-Modified-Since", IF_MODIFIED_SINCE},
        {"If-None-Match", IF_NONE_MATCH},
        {"If-Range", IF_RANGE},
        {"If-Unmodified-Since", IF_UNMODIFIED_SINCE},
        {"Max-Forwards", MAX_FORWARDS},
        {"Origin", ORIGIN},
        {"Pragma", PRAGMA},
        {"Proxy-Authorization", PROXY_AUTHORIZATION},
        {"Range", RANGE},
        {"Referer", REFERER},
        {"TE", TE},
        {"Upgrade", UPGRADE},
        {"Via", VIA},
        {"Warning", WARNING}
    };


    std::unordered_map<std::string, headers> HttpResponse::text_header = {
        {"", DEFAULT},
        {"Host", HOST},
        {"Content-Type", CONTENT_TYPE},
        {"Transfer-Encoding", TRANSFER_ENCODING},
        {"Content-Length", CONTENT_LENGTH},
        {"User-Agent", USER_AGENT},
        {"Accept", ACCEPT},
        {"Accept-Encoding", ACCEPT_ENCODING},
        {"Accept-Language", ACCEPT_LANGUAGE},
        {"Authorization", AUTHORIZATION},
        {"Cache-Control", CACHE_CONTROL},
        {"Connection", CONNECTION},
        {"Cookie", COOKIE},
        {"Date", DATE},
        {"Expect", EXPECT},
        {"From", FROM},
        {"If-Match", IF_MATCH},
        {"If-Modified-Since", IF_MODIFIED_SINCE},
        {"If-None-Match", IF_NONE_MATCH},
        {"If-Range", IF_RANGE},
        {"If-Unmodified-Since", IF_UNMODIFIED_SINCE},
        {"Max-Forwards", MAX_FORWARDS},
        {"Origin", ORIGIN},
        {"Pragma", PRAGMA},
        {"Proxy-Authorization", PROXY_AUTHORIZATION},
        {"Range", RANGE},
        {"Referer", REFERER},
        {"TE", TE},
        {"Upgrade", UPGRADE},
        {"Via", VIA},
        {"Warning", WARNING}
    };
}