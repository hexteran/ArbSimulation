#include <gtest/gtest.h>
#include "../src/simulation.hpp"

TEST(simulation, MarketDataSimulationManager_PublishesCorrectData)
{
    /*
    * Test verifies that MarketDataSimulationManager:
    * 1) sends sorted data
    * 2) sends all declared datasets
    * 3) sends correct message
    */
    using namespace ArbSimulation;

    struct MockSubscriber: public Subscriber
    {
        u_int64_t Timestamp = 0;
        bool IsOk = true;
        bool IsAPresent = false;
        bool IsBPresent = false;

        void OnNewMessage(MessagePtr message) final
        {
            if (message->Type != MessageType::L1Update)
                IsOk = false;

            auto m = std::static_pointer_cast<MDUpdateMessage>(message);
            if (IsOk == true)
                IsOk = m->Update->Timestamp >= this->Timestamp;
            
            if (m->Update->Instrument->SecurityId == "FutureA")
                IsAPresent = true;
            else if (m->Update->Instrument->SecurityId == "FutureB")
                IsBPresent = true;    
            else
                IsOk = false;
        }
    };
    auto sub = std::make_shared<MockSubscriber>();
    auto instrManager = std::make_shared<InstrumentManager>();
    MarketDataSimulationManager manager{instrManager, {"../../tests/data/csv_io_test_case_2.csv", "../../tests/data/csv_io_test_case_3.csv"}};
    manager.AddSubscriber(sub);
    while(manager.Step());
    
    EXPECT_TRUE(sub->IsOk);
    EXPECT_TRUE(sub->IsAPresent && sub->IsBPresent);
}

TEST(simulation, OrderMatcher_OneInstrumentTraded)
{
    /*
    * Test verifies that OrderMatcher:
    * 1) matches orders as expected
    * 2) sends back correct message
    */

    using namespace ArbSimulation;

    struct MockStrategy: public Subscriber
    {
        std::vector<OrderPtr> ordersSent;
        std::vector<OrderPtr> ordersConfirmed;
        InstrumentManager* manager;
        OrderMatcher* matcher;
        u_int64_t Timestamp = 0;
        bool isFirstOrderSent = false;
        void OnNewMessage(MessagePtr message) final
        {
            if ((message->Type == MessageType::L1Update && !isFirstOrderSent) 
            || message->Type == MessageType::OrderFilled)
            {
                isFirstOrderSent = true;
                auto newOrder = std::make_shared<Order>();
             
                newOrder->Instrument = message->Type == MessageType::L1Update ? 
                    std::static_pointer_cast<MDUpdateMessage>(message)->Update->Instrument:
                    std::static_pointer_cast<OrderFilledMessage>(message)->Order->Instrument;
                newOrder->Qty = 1;
                newOrder->Side = OrderSide::Sell;
                isFirstOrderSent = true;
                matcher->ProcessNewOrder(newOrder);
                ordersSent.push_back(newOrder);
            }
        }
    };
    std::unordered_map<std::string, u_int64_t> latencies({{"FutureA", 4}, {"FutureB", 4}});
    auto matcher = std::make_shared<OrderMatcher>(latencies);
    auto instrManager = std::make_shared<InstrumentManager>();
    auto sub = std::make_shared<MockStrategy>();
    sub->matcher = matcher.get();
    sub->manager = instrManager.get();
    MarketDataSimulationManager manager{instrManager, {"../../tests/data/order_matcher_test_1.csv"}};
    
    manager.AddSubscriber(matcher);
    matcher->AddSubscriber(sub);
    manager.AddSubscriber(sub);
    
    while(manager.Step());

    EXPECT_EQ(sub->ordersSent.size(), 7);
    EXPECT_EQ(sub->ordersSent[0]->ExecPrice, 998);
}

TEST(simulation, OrderMatcher_TwoInstrumentsTraded)
{
    /*
    * Test verifies that OrderMatcher:
    * 1) matches orders as expected
    * 2) sends back correct message
    * 3) adds correct latency
    */
    using namespace ArbSimulation;
    struct MockStrategy: public Subscriber
    {
        std::vector<OrderPtr> ordersSent;
        InstrumentManager* manager;
        OrderMatcher* matcher;
        u_int64_t Timestamp = 0;
        bool isFirstOrderSent = false;
        bool IsAWarmedUp = false;
        bool IsBWarmedUp = false;

        bool IsAOrderOnTheWay = false;
        bool IsBOrderOnTheWay = false;
        void OnNewMessage(MessagePtr message) final
        {
            if (message->Type == MessageType::L1Update)
            {
                const std::string& secId = std::static_pointer_cast<MDUpdateMessage>(message)->Update->Instrument->SecurityId;
                if (secId == "FutureA")
                    IsAWarmedUp = true;
                if (secId == "FutureB")
                    IsBWarmedUp = true;
            };

            if (message->Type == MessageType::OrderFilled)
            {
                const std::string& secId = std::static_pointer_cast<OrderFilledMessage>(message)->Order->Instrument->SecurityId;
                if (secId == "FutureA")
                    IsAOrderOnTheWay = false;
                if (secId == "FutureB")
                    IsBOrderOnTheWay = false;
            }

            if (IsAWarmedUp && IsBWarmedUp && !IsAOrderOnTheWay && !IsBOrderOnTheWay)
                if ((message->Type == MessageType::L1Update && !isFirstOrderSent) 
                || message->Type == MessageType::OrderFilled)
                {
                    isFirstOrderSent = true;
                    auto newOrderA = std::make_shared<Order>();
                    newOrderA->Instrument = manager->GetOrCreateInstrument("FutureA");
                    newOrderA->Qty = 1;
                    newOrderA->Side = OrderSide::Sell;
                    matcher->ProcessNewOrder(newOrderA);
                    ordersSent.push_back(newOrderA);
                    IsAOrderOnTheWay = true;

                    auto newOrderB = std::make_shared<Order>();
                    newOrderB->Instrument = manager->GetOrCreateInstrument("FutureB");
                    newOrderB->Qty = 1;
                    newOrderB->Side = OrderSide::Buy;
                    matcher->ProcessNewOrder(newOrderB);
                    ordersSent.push_back(newOrderB);
                    IsBOrderOnTheWay = true;
                }   
        }
    };
    
    std::unordered_map<std::string, u_int64_t> latencies({{"FutureA", 4}, {"FutureB", 4}});
    auto matcher = std::make_shared<OrderMatcher>(latencies);
    auto instrManager = std::make_shared<InstrumentManager>();
    auto sub = std::make_shared<MockStrategy>();
    sub->matcher = matcher.get();
    sub->manager = instrManager.get();
    MarketDataSimulationManager manager{instrManager, {"../../tests/data/order_matcher_test_1.csv", "../../tests/data/order_matcher_test_2.csv"}};
    
    manager.AddSubscriber(matcher);
    matcher->AddSubscriber(sub);
    manager.AddSubscriber(sub);
    
    while(manager.Step());
    int k = 1;
    EXPECT_EQ(sub->ordersSent.size(), 12);
    
    std::vector<double> prices{998, 1023, 997, 1023, 996, 1053, 990, 1056, 1000, 1052, 1001};
    std::vector<double> timestamps{1, 1, 7, 7, 13, 13, 19, 19, 25, 25, 31};
    
    for (int i = 0; i < prices.size(); ++i)
    {
        EXPECT_EQ(prices[i], sub->ordersSent[i]->ExecPrice);
        EXPECT_EQ(timestamps[i], sub->ordersSent[i]->SentTimestamp);
        EXPECT_EQ(timestamps[i] + 4, sub->ordersSent[i]->ExecutedTimestamp);
    }
}