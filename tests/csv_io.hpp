#include "definitions.h"
#include "../src/csv_io.hpp"

TEST(csv_io, read)
{
    using namespace ArbSimulation;
    auto result = CSVIO::ReadFile("../../tests/data/csv_io_test_case_1.csv");
    for (auto& line: result)
        EXPECT_EQ(line.size(), 7);

    EXPECT_EQ(result.size(), 22);

    EXPECT_EQ(result[3][0], "1544166006171067563");
    EXPECT_EQ(result[3][1], "FutureB");
    EXPECT_EQ(result[3][2], "2");
    EXPECT_EQ(result[3][3], "5");
    EXPECT_EQ(result[3][4], "10922");
    EXPECT_EQ(result[3][5], "10927");
    EXPECT_EQ(result[3][6], "3");
}
