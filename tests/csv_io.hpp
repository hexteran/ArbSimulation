#include <gtest/gtest.h>
#include "../src/csv_io.hpp"

TEST(csv_io, CSVIO_Read)
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

TEST(CSVIO, CSVIO_Write)
{
    using namespace ArbSimulation;
    std::vector<std::vector<std::string>> data{{"a", "b", "c"},{"d", "e", "g"}};
    CSVIO::WriteFile("../../tests/data/csv_io_write_test_case_1.csv", data);

    auto result = CSVIO::ReadFile("../../tests/data/csv_io_write_test_case_1.csv");
    for (auto& line: result)
        EXPECT_EQ(line.size(), 3);

    EXPECT_EQ(result.size(), 2);

    EXPECT_EQ(result[0][0], "a");
    EXPECT_EQ(result[0][1], "b");
    EXPECT_EQ(result[0][2], "c");
    EXPECT_EQ(result[1][0], "d");
    EXPECT_EQ(result[1][1], "e");
    EXPECT_EQ(result[1][2], "g");

    std::remove("../../tests/data/csv_io_write_test_case_1.csv"); 
}