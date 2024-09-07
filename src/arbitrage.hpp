#include "strategy_base.hpp"

namespace ArbSimulation
{
    class ArbitrageStrategy: public BasicStrategy
    {
    public:
        ArbitrageStrategy(double X, double Y, double Z, 
            std::shared_ptr<InstrumentManager> instrManager): 
            BasicStrategy(instrManager), 
            _parameterX(X), _parameterY(Y), _parameterZ(Z)
        {}

        void OnL1Update(L1UpdatePtr update) override
        {
            if(update->Instrument->SecurityId == "FutureA")
            {
                _lastAUpdate = update;
                return;
            }

            if (_lastAUpdate == nullptr)
            //do not act until we receive first update on FutureA
                return;

            if (!_isAOrderConfirmed || !_isBOrderConfirmed)
            //do not act while orders are pending
                return;

            auto& positionA = GetPosition("FutureA");
            auto& positionB = GetPosition("FutureB");
            
            auto a = positionA.GetNetQty();
            auto b = positionB.GetNetQty();

            if (positionA.GetNetQty() != -positionB.GetNetQty())
                throw StrategyException("Legs are not in sync");
            
            if (std::abs(positionA.GetNetQty()) > _parameterY)
                throw StrategyException("Max allowed qty is breached");

            if (_tradingRestricted)
                return;

            auto totalPnL = positionA.GetPnL() + positionB.GetPnL();

            if (positionA.GetNetQty() != 0 && totalPnL < _parameterZ)
            {
                std::cout << "\n\nSL is triggered";
                std::cout << "\n\tFutureA PnL:" << positionA.GetPnL() <<";FutureB PnL:"<< positionB.GetPnL() << "\n\n";
                SendSL("FutureA");
                SendSL("FutureB");
                _isAOrderConfirmed = false;
                _isBOrderConfirmed = false;
                _tradingRestricted = true;
                return;
            }

            if (positionA.GetNetQty() > -_parameterY && _lastAUpdate->BidPrice - update->AskPrice >= _parameterX)
            {
                //std::cout << "\n\nCondition A is met:\n\tFutureB.Bid:" << update->BidPrice  <<";FutureB.Ask:"<< update->AskPrice;
                //std::cout << "\n\tFutureA.Bid:" << _lastAUpdate->BidPrice  <<";FutureA.Ask:"<< _lastAUpdate->AskPrice;
                SendMarketOrder("FutureA", 1, OrderSide::Sell);
                SendMarketOrder("FutureB", 1, OrderSide::Buy);
                _isAOrderConfirmed = false;
                _isBOrderConfirmed = false;         
            }
            else if (positionA.GetNetQty() < _parameterY && update->BidPrice - _lastAUpdate->AskPrice >= _parameterX)
            {
                //std::cout << "\n\nCondition B is met:\n\tFutureB.Bid:" << update->BidPrice  <<";FutureB.Ask:"<< update->AskPrice;
                //std::cout << "\n\tFutureA.Bid:" << _lastAUpdate->BidPrice  <<";FutureA.Ask:"<< _lastAUpdate->AskPrice;
                SendMarketOrder("FutureA", 1, OrderSide::Buy);
                SendMarketOrder("FutureB", 1, OrderSide::Sell);
                _isAOrderConfirmed = false;
                _isBOrderConfirmed = false;               
            }
        }

        void OnOrderFilled(OrderPtr order) override
        {
            if (order->Instrument->SecurityId == "FutureA")
            {
                //std::cout << "\n\nFutureA, actual exec price: " << order->ExecPrice;
                _isAOrderConfirmed = true;
            }
            else if(order->Instrument->SecurityId == "FutureB")
            {
                //std::cout << "\n\nFutureB, actual exec price: " << order->ExecPrice;
                _isBOrderConfirmed = true;
            }
            else
                throw Exception("Unknown instrument");
        }

    private:
        L1UpdatePtr _lastAUpdate = nullptr;
        bool _isAOrderConfirmed = true;
        bool _isBOrderConfirmed = true;
        bool _tradingRestricted = false;
        double _parameterX = 0;
        double _parameterY = 0;
        double _parameterZ = 0;   
    };
}