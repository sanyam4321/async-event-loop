#pragma once

#include <functional>
#include <queue>
#include <utility>
#include <algorithm>
#include "socket_utilities.h"
#include "thread_safe_map.h"

using Callback = std::function<void()>;

namespace FiberConn
{
    enum EventPriority
    {
        LOW,
        HIGH
    };

    enum FDType
    {
        LISTEN_SOCK,
        NEW_SOCK
    };

    class IOReactor
    {
    private:
        int epollfd_;
        int max_events_;

        struct epoll_event *events_;

        struct PQComparator
        {
            bool operator()(const std::pair<EventPriority, struct epoll_event> &a,
                            const std::pair<EventPriority, struct epoll_event> &b) const
            {
                return a.first < b.first;
            }
        };

        std::priority_queue<std::pair<EventPriority, struct epoll_event>, std::vector<std::pair<EventPriority, struct epoll_event>>, PQComparator> event_queue_;
        ThreadSafeMap<int, Callback> event_callback_;
        ThreadSafeMap<int, FDType> event_type_;

    public:
        IOReactor(int max_events)
        {
            max_events_ = max_events;
            epollfd_ = FiberConn::getEpollInstance();
            events_ = new struct epoll_event[max_events_];
        }
        ~IOReactor()
        {
            delete events_;
        }

        int asyncAccept(int sockfd, Callback cb)
        {
            /*it will register the main listening socket and register a callback in the map*/
            /*set listen sock fdtype to LISTEN_SOCK*/
            /*set new sock fdtype to NEW_SOCK*/
        }
        int runCallback(std::pair<EventPriority, struct epoll_event> new_event)
        {
            /*get the callback from the hashmap and run it*/
            int temp_fd = new_event.second.data.fd;
            Callback cb;
            if (event_callback_.get(temp_fd, cb))
            {
                // Run the callback.
                cb();

                // Remove the callback from the map after running it.
                event_callback_.remove(temp_fd);
                return 0;
            }
            else
            {
                // If no callback was found, you could log an error or handle it accordingly.
                std::cerr << "No callback found for fd: " << temp_fd << std::endl;
                return -1;
            }
            /*after running the callback remove it from the map*/
        }
        int reactorRun()
        {

            while (true)
            {
                /*Dequeue all the pending event handlers*/
                while (!event_queue_.empty())
                {
                    /*Run the callbacks*/
                    runCallback(event_queue_.top());
                    event_queue_.pop();
                }

                /*Wait for new events only if event_callback size is not zero otherwise yield*/
                if (event_callback_.getSize() > 0)
                {
                    /*Epoll wait and push the sockets in the queue*/
                    int num_events = epoll_wait(epollfd_, events_, max_events_, -1);
                    if (num_events == -1)
                    {
                        std::cerr << "epoll wait error: " << strerror(errno) << std::endl;
                        return -1;
                    }

                    for (int i = 0; i < num_events; i++)
                    {
                        /*Push all these events to priority queue*/
                        int temp_fd = events_[i].data.fd;
                        EventPriority event_priority = LOW;
                        FDType temp_fd_type;
                        event_type_.get(temp_fd, temp_fd_type);
                        if (temp_fd_type == LISTEN_SOCK)
                        {
                            event_priority = LOW;
                        }
                        else if (temp_fd_type == NEW_SOCK)
                        {
                            event_priority = HIGH;
                        }
                        event_queue_.push(std::make_pair(event_priority, events_[i]));
                    }
                }
                else
                {
                    /*Dead reactor*/
                    return 0;
                }
            }
            return 0;
        }
    };
}