#include "definitions.h"
#include <simdjson.h>

struct Config
{
    double X = 0;
    double Y = 0;
    double Z = 0;
    std::unordered_map<std::string, u_int64_t> Latencies;
    std::vector<std::string> DataFiles;
};

int main(int argc, char* argv[])
{
    /*if (argc <= 1)
    {
        std::cout << "More arguments expected";
        return 0;
    }//*/

    simdjson::dom::parser parser;
    simdjson::dom::element parsed = parser.load("/home/hexteran/git/ArbSimulation/configs/default.json");
    simdjson::ondemand::parser ondParser;
    Config config;

    config.X = double(parsed["X"]);
    config.Y = double(parsed["Y"]);
    config.Z = double(parsed["Z"]);
    
    //parsed["Latencies"].get(config.Latencies);
    /*for(auto secId = parsed["Latencies"].begin(); secId != parsed["Latencies"].end(); ++secId)
    {
        int k = 1;
    }*/

    std::cout << "Done";
    
}