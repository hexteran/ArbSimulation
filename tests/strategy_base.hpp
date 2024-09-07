#include "definitions.h"
#include "../src/strategy_base.hpp"

TEST(Strategy, Position_Long)
{
    using namespace ArbSimulation;
    Position position1;
    position1.OnNewTrade(10, 100, OrderSide::Buy);
    position1.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position1.GetPnL(), 100);
    position1.OnNewTrade(10, 105, OrderSide::Buy);
    EXPECT_DOUBLE_EQ(position1.GetPnL(), 150);
    position1.OnNewTrade(20, 120, OrderSide::Sell);
    position1.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position1.GetPnL(), 350);
    EXPECT_DOUBLE_EQ(position1.GetNetQty(), 0);
    position1.OnNewTrade(10, 100, OrderSide::Buy);
    position1.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position1.GetPnL(), 450);
    position1.OnNewTrade(10, 105, OrderSide::Buy);
    EXPECT_DOUBLE_EQ(position1.GetPnL(), 500);
    position1.OnNewTrade(20, 120, OrderSide::Sell);
    position1.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position1.GetPnL(), 700);
}

TEST(Strategy, Position_Short)
{
    using namespace ArbSimulation;
    Position position2;
    position2.OnNewTrade(10, 100, OrderSide::Sell);
    position2.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position2.GetPnL(), -100);
    position2.OnNewTrade(10, 105, OrderSide::Sell);
    EXPECT_DOUBLE_EQ(position2.GetPnL(), -150);
    position2.OnNewTrade(20, 120, OrderSide::Buy);
    position2.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position2.GetPnL(), -350);
    EXPECT_DOUBLE_EQ(position2.GetNetQty(), 0);
    position2.OnNewTrade(10, 100, OrderSide::Sell);
    position2.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position2.GetPnL(), -450);
    position2.OnNewTrade(10, 105, OrderSide::Sell);
    EXPECT_DOUBLE_EQ(position2.GetPnL(), -500);
    position2.OnNewTrade(20, 120, OrderSide::Buy);
    position2.OnNewCurrentPrice(110);
    EXPECT_DOUBLE_EQ(position2.GetPnL(), -700);
}

TEST(Strategy, PositionKeeper_SingleInstrument)
{
    /*
    * Testing sequesnce of events:
    * CreatePosition->OpenLongPosition->ScaleBuy->Close->OpenShort->ScaleShort->Close
    */
    using namespace ArbSimulation;
    PositionKeeper keeper;
    InstrumentManager manager;
    auto instrument = manager.GetOrCreateInstrument("FutureA");

    auto update = std::make_shared<L1Update>();
    update->AskPrice = 111;
    update->BidPrice = 109;
    update->Instrument = instrument;
    keeper.OnL1Update(update);
    auto& positionA = keeper.GetPosition("FutureA");
    EXPECT_EQ(positionA.GetNetQty(), 0);
    EXPECT_EQ(positionA.GetPnL(), 0);

    auto order = std::make_shared<Order>();
    order->Instrument = instrument;
    order->ExecPrice = 100;
    order->Qty = 1;
    order->Side = OrderSide::Buy;
    keeper.OnOrderFilled(order);
    EXPECT_EQ(positionA.GetNetQty(), 1);
    EXPECT_EQ(positionA.GetPnL(), 10);

    order->Qty = 3;
    order->ExecPrice = 120;
    keeper.OnOrderFilled(order);
    EXPECT_EQ(positionA.GetNetQty(), 4);
    EXPECT_EQ(positionA.GetPnL(), -20);

    update->AskPrice = 121;
    update->BidPrice = 119;
    keeper.OnL1Update(update);
    EXPECT_EQ(positionA.GetNetQty(), 4);
    EXPECT_EQ(positionA.GetPnL(), 20);

    order->ExecPrice = 120;
    order->Qty = 4;
    order->Side = OrderSide::Sell;
    keeper.OnOrderFilled(order);
    EXPECT_EQ(positionA.GetNetQty(), 0);
    EXPECT_EQ(positionA.GetPnL(), 20);

    update->AskPrice = 101;
    update->BidPrice = 99;
    keeper.OnL1Update(update);
    EXPECT_EQ(positionA.GetNetQty(), 0);
    EXPECT_EQ(positionA.GetPnL(), 20);
    
    order->ExecPrice = 100;
    order->Qty = 4;
    order->Side = OrderSide::Sell;
    keeper.OnOrderFilled(order);
    EXPECT_EQ(positionA.GetNetQty(), -4);
    EXPECT_EQ(positionA.GetPnL(), 20);

    update->AskPrice = 91;
    update->BidPrice = 89;
    keeper.OnL1Update(update);
    EXPECT_EQ(positionA.GetNetQty(), -4);
    EXPECT_EQ(positionA.GetPnL(), 60);

    order->ExecPrice = 95;
    order->Qty = 4;
    order->Side = OrderSide::Sell;
    keeper.OnOrderFilled(order);
    EXPECT_EQ(positionA.GetNetQty(), -8);
    EXPECT_EQ(positionA.GetPnL(), 80);

    order->ExecPrice = 89;
    order->Qty = 8;
    order->Side = OrderSide::Buy;
    keeper.OnOrderFilled(order);
    EXPECT_EQ(positionA.GetNetQty(), 0);
    EXPECT_EQ(positionA.GetPnL(), 88);
}

TEST(Strategy, PositionKeeper_TwoInstrumentsTest)
{
    /*
    * Testing sequesnce of events:
    * Openning and closing of a spread position
    */
    using namespace ArbSimulation;
    PositionKeeper keeper;
    InstrumentManager manager;
    auto instrumentA = manager.GetOrCreateInstrument("FutureA");
    auto instrumentB = manager.GetOrCreateInstrument("FutureB");

    auto updateA = std::make_shared<L1Update>();
    updateA->AskPrice = 111;
    updateA->BidPrice = 109;
    updateA->Instrument = instrumentA;

    auto updateB = std::make_shared<L1Update>();
    updateB->AskPrice = 111;
    updateB->BidPrice = 109;
    updateB->Instrument = instrumentB;

    auto orderA = std::make_shared<Order>();
    orderA->Instrument = instrumentA;
    orderA->ExecPrice = 100;
    orderA->Qty = 1;
    orderA->Side = OrderSide::Buy;

    auto orderB = std::make_shared<Order>();
    orderB->Instrument = instrumentB;
    orderB->ExecPrice = 100;
    orderB->Qty = 1;
    orderB->Side = OrderSide::Buy;
///
    orderA->ExecPrice = 10927;
    orderA->SentTimestamp = 1544166008681726608;
    orderA->ExecutedTimestamp = 1544166008721726608;
    orderA->Qty = 1;
    orderA->Side = OrderSide::Sell;
    keeper.OnOrderFilled(orderA);

    orderB->ExecPrice = 10924;
    orderB->SentTimestamp = 1544166008681726608;
    orderB->ExecutedTimestamp = 1544166008682726608;
    orderB->Qty = 1;
    orderB->Side = OrderSide::Buy;
    keeper.OnOrderFilled(orderB);

    orderB->ExecPrice = 10903;
    orderB->SentTimestamp = 1544166655470444660;
    orderB->ExecutedTimestamp = 1544166655471444660;
    orderB->Qty = 1;
    orderB->Side = OrderSide::Sell;
    keeper.OnOrderFilled(orderB);

    orderA->ExecPrice = 10906;
    orderA->SentTimestamp = 1544166655470444660;
    orderA->ExecutedTimestamp = 1544166655510444660;
    orderA->Qty = 1;
    orderA->Side = OrderSide::Buy;
    keeper.OnOrderFilled(orderA);

    orderB->ExecPrice = 10871;
    orderB->SentTimestamp = 1544169548013367737;
    orderB->ExecutedTimestamp = 1544169548014367737;
    orderB->Qty = 1;
    orderB->Side = OrderSide::Buy;
    keeper.OnOrderFilled(orderB);

    orderA->ExecPrice = 10869.5;
    orderA->SentTimestamp = 1544169548013367737;
    orderA->ExecutedTimestamp = 1544169548053367737;
    orderA->Qty = 1;
    orderA->Side = OrderSide::Sell;
    keeper.OnOrderFilled(orderA);

    orderB->ExecPrice = 10870;
    orderB->SentTimestamp = 1544195871070727486;
    orderB->ExecutedTimestamp = 1544195871071727486;
    orderB->Qty = 1;
    orderB->Side = OrderSide::Sell;
    keeper.OnOrderFilled(orderB);

    orderA->ExecPrice = 10871.0;
    orderA->SentTimestamp = 1544195871070727486;
    orderA->ExecutedTimestamp = 1544195871110727486;
    orderA->Qty = 1;
    orderA->Side = OrderSide::Buy;
    keeper.OnOrderFilled(orderA);

    auto pnl = keeper.GetFullPnL();
    EXPECT_EQ(pnl, -2.5);
}

