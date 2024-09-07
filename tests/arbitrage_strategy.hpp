#pragma once
#include "definitions.h"
#include "../src/arbitrage.hpp"
TEST(ArbitrageStrategy, ShortRun)
{
    using namespace ArbSimulation;
    auto instrManager = std::make_shared<InstrumentManager>();
    std::vector<std::string> datasets{"../../tests/data/arb_strategy_test_A.csv", "../../tests/data/arb_strategy_test_B.csv"};
    //std::vector<std::string> datasets{"/home/hexteran/Downloads/arbitrageFutureAData.csv", "/home/hexteran/Downloads/arbitrageFutureBData.csv"};
    auto marketDataManager = std::make_shared<MarketDataSimulationManager>(instrManager, datasets);
     std::unordered_map<std::string, u_int64_t> latencies({{"FutureA", 0}, {"FutureB", 0}});
    auto orderMatcher = std::make_shared<OrderMatcher>(latencies);
    auto arbStrategy = std::make_shared<ArbitrageStrategy>(5, 2, -150, instrManager);    
   
    arbStrategy->AddSubscriber(orderMatcher);
    orderMatcher->AddSubscriber(arbStrategy);
    marketDataManager->AddSubscriber(orderMatcher);
    marketDataManager->AddSubscriber(arbStrategy);

    while(marketDataManager->Step());

    EXPECT_EQ(arbStrategy->GetFullPnL(), 77);
}