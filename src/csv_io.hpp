#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <fstream>

namespace ArbSimulation
{
    class CSVIO
    {
    public:
        static std::vector<std::vector<std::string>> ReadFile(const std::string& path, char sep = ',')
        {
            std::vector<std::vector<std::string>> result;
            std::ifstream file{path};
            std::string line;
            while (std::getline(file, line, '\n'))
            {
                std::vector<std::string> elems; 
                std::stringstream ss{line};
                std::string elem;
                while (std::getline(ss, elem, ','))
                    elems.push_back(elem);
                result.push_back(elems);
            }
            file.close();
            return result;
        }

        static void WriteFile(const std::string& path, const std::vector<std::vector<std::string>>& data, char sep = ',')
        {
            std::ofstream file{path};
            for(auto& line: data)
            {
                std::string l{};
                for(auto& elem: line)
                {
                    l += elem + sep;
                }
                l[l.size() - 1] = '\n';
                file << l;
            }
            file.close();
        }
    };
}