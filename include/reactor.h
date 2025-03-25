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
        }
        int runCallback(std::pair<EventPriority, struct epoll_event>)
        {
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
                        event_queue_.push(std::make_pair(LOW, events_[i]));
                    }
                }
                else
                {
                    /*Dead reactor*/
                    return 0;
                }

                /*Enqueue ready event handlers*/
            }
            return 0;
        }
    };
}