#pragma once
#include "simulation.hpp"

namespace ArbSimulation
{
    class Position
    {
    public:
        inline double GetNetQty() const
        {
            return _netQty;
        }

        inline double GetPnL() const 
        {
            /*
            * This implementation differs from what was described in the doc
            * But meaning is the same: unrealized pnl + realized pnl
            */
            if (_netQty >= 0)
            {
                if (_qtyBought == 0)   
                    return 0;
                double avgPriceSold = _avgPriceSold * _qtySold / (_qtySold + _netQty) + _currentPrice * _netQty / (_qtySold + _netQty);
                return  std::round((avgPriceSold - _avgPriceBought)*std::max(_qtySold, _qtyBought)*MAX_PRECISION/MAX_PRECISION);
            }
            else
            {
                if (_qtySold == 0)   
                    return 0;
                double avgPriceBought = _avgPriceBought * _qtyBought / _qtySold + _currentPrice * (-_netQty) / _qtySold;
                return  std::round((_avgPriceSold - avgPriceBought)*std::max(_qtySold, _qtyBought)*MAX_PRECISION/MAX_PRECISION);
            }
        }

        inline void OnNewCurrentPrice(double price)
        {
            _currentPrice = price;
        }

        void OnNewTrade(double qty, double price, OrderSide side)
        {
            switch(side)
            {
                case OrderSide::Buy:
                {
                    _avgPriceBought = _avgPriceBought * _qtyBought / (_qtyBought + qty) + price * qty / (_qtyBought + qty);
                    _qtyBought += qty;
                    break;
                }
                case OrderSide::Sell:
                {
                    _avgPriceSold = _avgPriceSold * _qtySold / (_qtySold + qty) + price * qty / (_qtySold + qty);
                    _qtySold += qty;
                    break;
                }
                default:
                    throw Exception("Wrong value");
            }
            _netQty = _qtyBought - _qtySold;
        }

    private:
        double _qtyBought = 0;
        double _qtySold = 0;
        double _netQty = 0;
        double _avgPriceBought = 0;
        double _avgPriceSold = 0;
        double _currentPrice = 0;
        
    };

    class PositionKeeper
    {
    public:
        inline const Position& GetPosition(const std::string& securityId)
        {
            if (_positionsMap.find(securityId) == _positionsMap.end())
                _positionsMap.insert({securityId, Position()});

            return _positionsMap[securityId];
        }

        inline void OnL1Update(L1UpdatePtr update)
        {
            const std::string& secId = update->Instrument->SecurityId;
            _getOrCreatePosition(secId).OnNewCurrentPrice((update->BidPrice + update->AskPrice)/2);
        };

        inline void OnOrderFilled(OrderPtr order)
        {
            const std::string& secId = order->Instrument->SecurityId;
            _getOrCreatePosition(secId).OnNewTrade(order->Qty, order->ExecPrice, order->Side);
        };

        void OnNewMessage(MessagePtr message)
        {
            switch (message->Type)
            {
                case (MessageType::L1Update):
                {
                    OnL1Update(std::static_pointer_cast<MDUpdateMessage>(message)->Update);
                    break;
                }
                case (MessageType::OrderFilled):
                {
                    OnOrderFilled(std::static_pointer_cast<OrderFilledMessage>(message)->Order);
                    break;
                }
            }
        }
    private:
        Position& _getOrCreatePosition(const std::string& securityId)
        {
            auto iter = _positionsMap.find(securityId);
            if (iter == _positionsMap.end())
            {
                _positionsMap.insert({securityId, Position()});
                return _positionsMap[securityId];
            }
            return iter->second;
        }

    private:
        std::unordered_map<std::string, Position> _positionsMap;
    };

    class BasicStrategy: public Subscriber, public Publisher
    {
    public:
        BasicStrategy(std::shared_ptr<InstrumentManager> instrManager):_instrManager(instrManager)
        {}

        virtual void OnL1Update(L1UpdatePtr update) = 0;
        virtual void OnOrderFilled(OrderPtr order) = 0;

        void SendSL(const std::string& securityId)
        {
            auto& position = _positionKeeper.GetPosition(securityId);
            auto order = std::make_shared<Order>();
            order->Qty = std::abs(position.GetNetQty());
            order->Side = position.GetNetQty() > 0 ? OrderSide::Sell: OrderSide::Buy;
            order->Type = OrderType::StopLoss;
            order->Instrument = _instrManager->GetOrCreateInstrument(securityId);
            if (order->Qty > 0)
            {
                auto message = std::make_shared<NewOrderMessage>();
                message->Order = order;
                SendMessage(message);
            }
        }

        void SendMarketOrder(const std::string& securityId, double qty, OrderSide side)
        {
            auto order = std::make_shared<Order>();
            order->Qty = qty;
            order->Side = side;
            order->Type = OrderType::Market;
            order->Instrument = _instrManager->GetOrCreateInstrument(securityId);
            auto message = std::make_shared<NewOrderMessage>();
            message->Order = order;
            SendMessage(message);
        }

        inline const Position& GetPosition(const std::string& securityId)
        {
            return _positionKeeper.GetPosition(securityId);
        }

        void OnNewMessage(MessagePtr message)
        {
            switch(message->Type)
            {
                case (MessageType::L1Update):
                {
                    _positionKeeper.OnL1Update(std::static_pointer_cast<MDUpdateMessage>(message)->Update);
                    OnL1Update(std::static_pointer_cast<MDUpdateMessage>(message)->Update);
                    break;
                }
                case (MessageType::OrderFilled):
                {
                    _positionKeeper.OnOrderFilled(std::static_pointer_cast<OrderFilledMessage>(message)->Order);
                    OnOrderFilled(std::static_pointer_cast<OrderFilledMessage>(message)->Order);
                    break;
                }
                default:
                    throw Exception("Unexpected MessageType");
            }
        }

    private:
        std::shared_ptr<InstrumentManager> _instrManager;
        PositionKeeper _positionKeeper;
    };
}