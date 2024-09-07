#pragma once
#include "csv_io.hpp"
#include "observer.hpp"

namespace ArbSimulation
{
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

    struct MDUpdateMessage: public Message
    {
        L1UpdatePtr Update;
        MDUpdateMessage()
        {
            Type = MessageType::L1Update;
        }
    };

    class InstrumentManager
    {
    public:         
        InstrumentPtr GetOrCreateInstrument(const std::string& securityId)
        {
            auto iter = _instruments.find(securityId);
            if (iter == _instruments.end())
            {
                auto result = std::make_shared<Instrument>();
                result -> SecurityId = securityId;
                _instruments[securityId] = result;
                return result;
            }
            return iter->second;
        }

    private:
        std::unordered_map<std::string, InstrumentPtr> _instruments;
    };

    class MarketDataSimulationManager: public Publisher
    {
    public:
        MarketDataSimulationManager();
        MarketDataSimulationManager(MarketDataSimulationManager&&) = delete;
        MarketDataSimulationManager(MarketDataSimulationManager&) = delete;
        MarketDataSimulationManager(const MarketDataSimulationManager&) = delete;
        MarketDataSimulationManager(std::shared_ptr<InstrumentManager> instrManager, std::vector<std::string> paths):
        _instrumentManager(instrManager)
        {
            for (auto& path: paths)
                _loadData(path);
            _sortData();
        }

        bool Step()
        {
            if (_cursor < _data.size())
            {
                auto message = std::make_shared<MDUpdateMessage>();
                message->Update = _data[_cursor];
                ++_cursor;
                SendMessage(message);
                return true;
            }
            return false;
        }

    private:

        void _loadData(const std::string& path)
        {
            auto rawLines = CSVIO::ReadFile(path);
            for(auto& line: rawLines)
            {
                auto update = std::make_shared<L1Update>();
                update->Instrument = _instrumentManager->GetOrCreateInstrument(line[1]);
                update->Timestamp = std::stol(line[0]);
                update->BidSize = std::stod(line[3]);
                update->BidPrice = std::stod(line[4]);
                update->AskPrice = std::stod(line[5]);
                update->AskSize = std::stod(line[6]);
                _data.push_back(update);
            }
        }

        inline void _sortData()
        {
            //Quite time consuming, but this application is not latency-sensitive
            std::sort(_data.begin(), _data.end(), [](L1UpdatePtr& a, L1UpdatePtr& b){ return a->Timestamp < b->Timestamp;});
        }

    private:
        std::vector<L1UpdatePtr> _data;
        std::shared_ptr<InstrumentManager> _instrumentManager;
        int _cursor{0};
        int _size{0};
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

    class OrderMatcher: public Subscriber, public Publisher
    {
    public:
        OrderMatcher() = delete;
        OrderMatcher(OrderMatcher&) = delete;
        OrderMatcher(const OrderMatcher&) = delete;
        OrderMatcher(OrderMatcher&&) = delete;

        OrderMatcher(const std::unordered_map<std::string, u_int64_t>& latencies): _latencies(latencies)
        {
        }

        void ProcessNewOrder(OrderPtr order)
        {
            //putting orders into the queue in order to check on upcoming md updates
            std::string& securityId = order->Instrument->SecurityId;
            order->SentTimestamp = _currentTimestamp;

            auto iter = _orderQueues.find(securityId);
        
            if (iter == _orderQueues.end())
            {
                _orderQueues.insert({securityId, std::queue<OrderPtr>{}});
                _orderQueues[securityId].push(order);
            }
            else
                iter->second.push(order);
        }

        void ProcessL1Update(L1UpdatePtr update)
        {
            _currentTimestamp = update->Timestamp;
            std::string &securityId = update->Instrument->SecurityId;

            auto iterUpdates = _lastUpdates.find(securityId);

            auto iterQueues = _orderQueues.find(securityId);
            if (iterQueues != _orderQueues.end() && iterUpdates != _lastUpdates.end())
            {
                while (!iterQueues->second.empty() && iterQueues->second.front()->SentTimestamp + _latencies[securityId] < update->Timestamp)
                {
                    auto message = std::make_shared<OrderFilledMessage>();
                    message->Order = iterQueues->second.front();
                    double execPrice = message->Order->Side == OrderSide::Buy ? iterUpdates->second->AskPrice : iterUpdates->second->BidPrice;
                    if (message->Order->Type == OrderType::StopLoss)
                        execPrice = (iterUpdates->second->AskPrice + iterUpdates->second->BidPrice) / 2;

                    message->Order->ExecPrice = execPrice;
                    message->Order->ExecutedTimestamp = iterQueues->second.front()->SentTimestamp + _latencies[securityId]; // iterUpdates->second->Timestamp;
                    iterQueues->second.pop();
                    SendMessage(message);
                }
            }
            _lastUpdates[securityId] = update;
        }

        void OnNewMessage(MessagePtr message)
        {
            //we should get only MDUpdates, any other message types are restricted
            switch(message->Type)
            {
                case MessageType::L1Update:
                {
                    ProcessL1Update(std::static_pointer_cast<MDUpdateMessage>(message)->Update);
                    break;
                }
                case MessageType::NewOrder:
                {
                    ProcessNewOrder(std::static_pointer_cast<NewOrderMessage>(message)->Order);
                    break;
                }
                default:
                    throw MessagingError("Unexpected MessageType");
                
            }
        }

    private:
        u_int64_t _currentTimestamp{0};
        std::unordered_map<std::string, std::queue<OrderPtr>> _orderQueues;
        std::unordered_map<std::string, L1UpdatePtr> _lastUpdates;
        std::unordered_map<std::string, u_int64_t> _latencies;
        int _latency{0};
    };
}