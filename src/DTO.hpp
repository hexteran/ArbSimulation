/*
* This file contains declarations of different entities
*/

#pragma once

#include "exceptions.hpp"

namespace ArbSimulation
{
// Enums
    enum class MessageType
    {
        L1Update,
        NewOrder,
        OrderFilled
    };

    enum class OrderSide
    {
        Buy,
        Sell
    };

    enum class OrderType
    {
        Market,
        StopLoss
    };

// Basic structures

    struct Message
    {
        MessageType Type;    
    };
    typedef std::shared_ptr<Message> MessagePtr;

    struct Instrument
    {
        std::string SecurityId;
        double PriceStep = 1;
    }; 
    typedef std::shared_ptr<Instrument> InstrumentPtr;

    struct L1Update
    {
        u_int64_t Timestamp;
        InstrumentPtr Instrument;
        double BidSize;
        double BidPrice;
        double AskSize;
        double AskPrice;
    };
    typedef std::shared_ptr<L1Update> L1UpdatePtr;

    struct Order
    {
        InstrumentPtr Instrument;
        double Qty;
        OrderSide Side;
        double ExecPrice = 0;
        u_int64_t SentTimestamp = 0;
        u_int64_t ExecutedTimestamp = 0;
        OrderType Type = OrderType::Market;
    };
    typedef std::shared_ptr<Order> OrderPtr;

// Messages

    struct MDUpdateMessage: public Message
    {
        L1UpdatePtr Update;
        MDUpdateMessage()
        {
            Type = MessageType::L1Update;
        }
    };

    struct NewOrderMessage: public Message
    {
        OrderPtr Order;
        NewOrderMessage()
        {
            Type = MessageType::NewOrder;
        }
    };

    struct OrderFilledMessage: public Message
    {
        OrderPtr Order;
        OrderFilledMessage()
        {
            Type = MessageType::OrderFilled;
        }
    };

    

}