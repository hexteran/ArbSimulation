#include <simdjson.h>
#include <chrono>

#include "arbitrage.hpp"

struct Config
{
    double X = 0;
    double Y = 0;
    double Z = 0;
    std::unordered_map<std::string, u_int64_t> Latencies;
    std::vector<std::string> DataFiles;
    std::string ReportsFolder;

    bool Loaded = false;

    Config(const std::string& configPath)
    {
        try
        {
            std::cout << "Reading " << configPath<< ":\n";
            simdjson::dom::parser parser;
            simdjson::dom::object object;

            auto error = parser.load(configPath).get(object);

            X = double(object["X"]);
            Y = double(object["Y"]);
            Z = double(object["Z"]);
            std::cout << "\tParameter X = " << X << "\n";
            std::cout << "\tParameter Y = " << Y << "\n";
            std::cout << "\tParameter Z = " << Z << "\n";

            simdjson::dom::object latencies = object["Latencies"].get_object();
            std::cout << "\tLatencies:\n";
            for (auto [key, value] : latencies)
            {  
                std::cout << "\t\t" << std::string(key) << ": " << u_int64_t(value) << "\n";
                Latencies.insert({std::string(key), u_int64_t(value)});
            }

            simdjson::dom::array dataFiles = object["DataFiles"].get_array();
            std::cout << "\tDataFiles:\n";
            for (auto value : dataFiles)
            {
                std::cout << "\t\t" << std::string(value) << "\n";
                DataFiles.push_back(std::string(value));
            }
            
            ReportsFolder = std::string(object["Reports"]);
            std::cout << "\tReportsFolder: " << ReportsFolder << "\n\n";
            Loaded = true;
        }
        catch(std::exception& ex)
        {
            std::cerr << ex.what();
        }
    }
};

int main(int argc, char* argv[])
{
    using namespace ArbSimulation;

    std::string configPath = "../../configs/default.json";
    if (argc > 1)
        configPath = argv[1];

    Config config(configPath);

    if (!config.Loaded)
        return -1;
    
    auto instrManager = std::make_shared<InstrumentManager>();

    std::cout << "Loading data\n\n";
    auto marketDataManager = std::make_shared<MarketDataSimulationManager>(instrManager, config.DataFiles);
    auto orderMatcher = std::make_shared<OrderMatcher>(config.Latencies);
    auto arbStrategy = std::make_shared<ArbitrageStrategy>(config.X, config.Y, config.Z, instrManager);    
    
    arbStrategy->AddSubscriber(orderMatcher);
    orderMatcher->AddSubscriber(arbStrategy);
    marketDataManager->AddSubscriber(orderMatcher);
    marketDataManager->AddSubscriber(arbStrategy);

    std::cout << "Running simulation...\n\n";
    while(marketDataManager->Step());

    std::cout << "Simulation is done!\n***\n\tFinal PnL is " << arbStrategy->GetFullPnL() << '\n';//*/
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);
    char datetime[256];
    strftime(datetime, 1024, "%F_%T", &now_tm);
    std::string filename = config.ReportsFolder + 
        (config.ReportsFolder.back() != '/' ? "/": "") + 
        "trades_" + datetime + ".csv";
    
    auto& trades = arbStrategy->GetTrades();
    std::vector<std::vector<std::string>> reportLines{
        {"SecurityId", 
        "SentTimestamp", 
        "ExecutedTimestamp", 
        "ExecPrice", 
        "Qty", 
        "Side", 
        "Type"}};

    for (auto& trade: trades)
    {
        std::vector<std::string> line;
        line.push_back(trade->Instrument->SecurityId);
        line.push_back(std::to_string(trade->SentTimestamp));
        line.push_back(std::to_string(trade->ExecutedTimestamp));
        line.push_back(std::to_string(trade->ExecPrice));
        line.push_back(std::to_string(trade->Qty));
        line.push_back(trade->Side == OrderSide::Buy ? "BUY" : "SELL");
        line.push_back(trade->Type == OrderType::StopLoss ? "StopLoss" : "Market");
        reportLines.push_back(line);
    }

    CSVIO::WriteFile(filename, reportLines, ';');

    std::cout << "\tTrades are saved: " + filename + "\n";
    return 0; 
}