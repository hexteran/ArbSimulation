#include "simulation.hpp"

namespace ArbSimulation
{
    class Position
    {
    public:
        inline double GetNetQty()
        {
            return _netQty;
        }

        inline double GetPnL()
        {
            return _unrealizedPnL + _realizedPnL;
        }

        void OnNewCurrentPrice(double price)
        {
            _currentPrice = price;
            _unrealizedPnL = _calcUnrealizedPnL();
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
            _unrealizedPnL = _calcUnrealizedPnL();
            _realizedPnL = _calcRealizedPnL();
        }

    private:
        inline double _calcUnrealizedPnL()
        {
            double result = _netQty > 0 ?
                _currentPrice * _netQty / _qtyBought + _avgPriceSold * _qtySold / _qtyBought:
                -_currentPrice * _netQty / _qtySold + _avgPriceSold * _qtyBought / _qtySold;
        }

        inline double _calcRealizedPnL()
        {
            return (_avgPriceSold - _avgPriceBought)*std::min(_qtyBought, _qtySold);
        }

    private:
        double _qtyBought = 0;
        double _qtySold = 0;
        double _unrealizedPnL = 0;
        double _realizedPnL = 0;
        double _netQty = 0;
        double _avgPriceBought = 0;
        double _avgPriceSold = 0;
        double _pnl = 0;
        double _currentPrice = 0;
    };

    class PositionKeeper: public Subscriber
    {
    public:
        PositionKeeper(std::shared_ptr<InstrumentManager> instrManager): 
        _instrumentManager(instrManager)
        {}

        void OnL1Update(L1UpdatePtr update)
        {
            const std::string& secId = update->Instrument->SecurityId;
            double priceStep = update->Instrument->PriceStep;
            auto iter = _positionsMap.find(secId);
            if (iter != _positionsMap.end())
                iter->second.back().OnNewCurrentPrice((update->BidPrice + update->AskPrice)/2);
        };

        void OnOrderFilled(OrderPtr order)
        {
            const std::string& secId = order->Instrument->SecurityId;
            auto iter = _positionsMap.find(secId);
            if (iter == _positionsMap.end())
            {
                _positionsMap.insert({secId, std::vector<Position>()});
            }

            auto& positionsOnInstr = _positionsMap[secId];
            if (positionsOnInstr.empty() || positionsOnInstr.back().GetNetQty() == 0)
                positionsOnInstr.push_back(Position());
            
            positionsOnInstr.back().OnNewTrade(order->Qty, order->ExecPrice, order->Side);
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
        std::shared_ptr<InstrumentManager> _instrumentManager;
        std::unordered_map<std::string, std::vector<Position>> _positionsMap;
    };

    class BasicStrategy: public Subscriber
    {
    public:
        virtual void OnL1Update(L1UpdatePtr update) = 0;

        virtual void OnOrderFilled(OrderPtr order) = 0;

        void OnNewMessage(MessagePtr message)
        {
            switch(message->Type)
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
    };
}