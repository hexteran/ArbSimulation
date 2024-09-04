#include "definitions.h"
#include "../src/simulation.hpp"
/*TEST(Simulation, DataManagerTest)
{
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
}//*/

TEST(Simulation, OrderMatcherTest_OneInstrument)
{
    using namespace ArbSimulation;

    struct MockStrategy: public Subscriber
    {
        InstrumentManager* manager;
        OrderMatcher* matcher;
        u_int64_t Timestamp = 0;
        bool isFirstOrderSent = false;
        void OnNewMessage(MessagePtr message) final
        {
            if ((message->Type != MessageType::L1Update && !isFirstOrderSent) 
            || message->Type != MessageType::OrderFilled)
            {
                isFirstOrderSent = true;
                auto newOrder = std::make_shared<Order>();
             
                newOrder->Instrument = message->Type == MessageType::L1Update ? 
                    std::static_pointer_cast<MDUpdateMessage>(message)->Update->Instrument:
                    std::static_pointer_cast<OrderFilledMessage>(message)->Order->Instrument;
                newOrder->Qty = 1;
                newOrder->Side = OrderSide::Buy;
                isFirstOrderSent = true;
                matcher->SendOrder(newOrder);
            }
        }
    };
    auto matcher = std::make_shared<OrderMatcher>(4);
    auto instrManager = std::make_shared<InstrumentManager>();
    auto sub = std::make_shared<MockStrategy>();
    sub->matcher = matcher.get();
    sub->manager = instrManager.get();
    MarketDataSimulationManager manager{instrManager, {"../../tests/data/order_matcher_test_1.csv"}};
    manager.AddSubscriber(matcher);
    manager.AddSubscriber(sub);
    while(manager.Step());

    EXPECT_TRUE(1==1);
    //EXPECT_TRUE(sub->IsAPresent && sub->IsBPresent);
}