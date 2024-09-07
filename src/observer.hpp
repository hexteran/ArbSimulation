#pragma once 

#include "DTO.hpp"

namespace ArbSimulation
{
    class Subscriber
    {
    public:
        virtual void OnNewMessage(MessagePtr message)
        {}
    };

    class Publisher
    {
    public:
        inline void SendMessage(MessagePtr message)
        {
            for(auto& subscriber: _subscribers)
                subscriber->OnNewMessage(message);
        }

        inline void AddSubscriber(std::shared_ptr<Subscriber> subscriber)
        {
            _subscribers.push_back(subscriber);
        }
    
    private:
        std::vector<std::shared_ptr<Subscriber>> _subscribers;
    };
};