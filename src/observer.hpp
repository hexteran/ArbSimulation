#pragma once
#ifndef DEFINITIONS
#include "definitions.h"
#endif

namespace ArbSimulation
{
    enum class MessageType
    {
        L1Update,
        NewOrder,
        OrderFilled
    };

    struct Message
    {
        MessageType Type;    
    };
    
    typedef std::shared_ptr<Message> MessagePtr;

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