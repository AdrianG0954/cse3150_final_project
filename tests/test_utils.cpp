#include <gtest/gtest.h>
#include "Utils.h"
#include <vector>
#include <string>

class UtilsTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== SPLIT FUNCTION TESTS ====================

TEST_F(UtilsTest, SplitBasic)
{
    std::vector<std::string> result = Utils::split("a|b|c", '|');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ("a", result[0]);
    EXPECT_EQ("b", result[1]);
    EXPECT_EQ("c", result[2]);
}

TEST_F(UtilsTest, SplitEmptyString)
{
    std::vector<std::string> result = Utils::split("", '|');
    EXPECT_EQ(0, result.size());
}

TEST_F(UtilsTest, SplitSingleToken)
{
    std::vector<std::string> result = Utils::split("single", '|');
    ASSERT_EQ(1, result.size());
    EXPECT_EQ("single", result[0]);
}

TEST_F(UtilsTest, SplitTrailingDelimiter)
{
    std::vector<std::string> result = Utils::split("a|b|", '|');
    ASSERT_EQ(2, result.size());
    EXPECT_EQ("a", result[0]);
    EXPECT_EQ("b", result[1]);
}

TEST_F(UtilsTest, SplitLeadingDelimiter)
{
    std::vector<std::string> result = Utils::split("|a|b", '|');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ("", result[0]); // Empty first token
    EXPECT_EQ("a", result[1]);
    EXPECT_EQ("b", result[2]);
}

TEST_F(UtilsTest, SplitMultipleConsecutiveDelimiters)
{
    std::vector<std::string> result = Utils::split("a||b", '|');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ("a", result[0]);
    EXPECT_EQ("", result[1]); // Empty middle token
    EXPECT_EQ("b", result[2]);
}

TEST_F(UtilsTest, SplitDifferentDelimiters)
{
    std::vector<std::string> result1 = Utils::split("a,b,c", ',');
    ASSERT_EQ(3, result1.size());

    std::vector<std::string> result2 = Utils::split("a b c", ' ');
    ASSERT_EQ(3, result2.size());

    std::vector<std::string> result3 = Utils::split("a:b:c", ':');
    ASSERT_EQ(3, result3.size());
}

TEST_F(UtilsTest, SplitNoDelimiterPresent)
{
    std::vector<std::string> result = Utils::split("nodelimiterhere", '|');
    ASSERT_EQ(1, result.size());
    EXPECT_EQ("nodelimiterhere", result[0]);
}

TEST_F(UtilsTest, SplitLongString)
{
    std::string longStr = "a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p";
    std::vector<std::string> result = Utils::split(longStr, '|');
    EXPECT_EQ(16, result.size());
    EXPECT_EQ("a", result[0]);
    EXPECT_EQ("p", result[15]);
}

TEST_F(UtilsTest, SplitCAIDAFormat)
{
    // Test with actual CAIDA format line
    std::string line = "100|200|-1|bgp";
    std::vector<std::string> result = Utils::split(line, '|');
    ASSERT_EQ(4, result.size());
    EXPECT_EQ("100", result[0]);
    EXPECT_EQ("200", result[1]);
    EXPECT_EQ("-1", result[2]);
    EXPECT_EQ("bgp", result[3]);
}

TEST_F(UtilsTest, SplitWithNumbers)
{
    std::vector<std::string> result = Utils::split("123|456|789", '|');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ("123", result[0]);
    EXPECT_EQ("456", result[1]);
    EXPECT_EQ("789", result[2]);
}

TEST_F(UtilsTest, SplitWithWhitespace)
{
    std::vector<std::string> result = Utils::split("a | b | c", '|');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ("a ", result[0]);  // Whitespace preserved
    EXPECT_EQ(" b ", result[1]); // Whitespace preserved
    EXPECT_EQ(" c", result[2]);  // Whitespace preserved
}

// ==================== JOIN FUNCTION TESTS ====================

TEST_F(UtilsTest, JoinBasicIntegers)
{
    std::vector<int> vec = {1, 2, 3};
    std::string result = Utils::join(vec, ", ");
    EXPECT_EQ("1, 2, 3", result);
}

TEST_F(UtilsTest, JoinSingleElement)
{
    std::vector<int> vec = {42};
    std::string result = Utils::join(vec, ", ");
    EXPECT_EQ("42", result);
}

TEST_F(UtilsTest, JoinEmptyVector)
{
    std::vector<int> vec;
    std::string result = Utils::join(vec, ", ");
    EXPECT_EQ("", result);
}

TEST_F(UtilsTest, JoinWithDifferentSeparators)
{
    std::vector<int> vec = {10, 20, 30};

    std::string result1 = Utils::join(vec, "-");
    EXPECT_EQ("10-20-30", result1);

    std::string result2 = Utils::join(vec, " | ");
    EXPECT_EQ("10 | 20 | 30", result2);

    std::string result3 = Utils::join(vec, "");
    EXPECT_EQ("102030", result3);
}

TEST_F(UtilsTest, JoinStrings)
{
    std::vector<std::string> vec = {"hello", "world", "test"};
    std::string result = Utils::join(vec, " ");
    EXPECT_EQ("hello world test", result);
}

TEST_F(UtilsTest, JoinLargeVector)
{
    std::vector<int> vec;
    for (int i = 0; i < 100; ++i)
    {
        vec.push_back(i);
    }
    std::string result = Utils::join(vec, ",");

    // Check that it starts and ends correctly
    EXPECT_TRUE(result.substr(0, 2) == "0,");
    EXPECT_TRUE(result.substr(result.length() - 2) == "99");
}

TEST_F(UtilsTest, JoinASPath)
{
    // Simulating AS path joining
    std::vector<int> asPath = {100, 200, 300, 400};
    std::string result = Utils::join(asPath, " ");
    EXPECT_EQ("100 200 300 400", result);
}

TEST_F(UtilsTest, JoinNegativeNumbers)
{
    std::vector<int> vec = {-1, 0, 1, -5, 10};
    std::string result = Utils::join(vec, ", ");
    EXPECT_EQ("-1, 0, 1, -5, 10", result);
}

TEST_F(UtilsTest, JoinWithNewlineSeparator)
{
    std::vector<int> vec = {1, 2, 3};
    std::string result = Utils::join(vec, "\n");
    EXPECT_EQ("1\n2\n3", result);
}

TEST_F(UtilsTest, JoinTwoElements)
{
    std::vector<int> vec = {100, 200};
    std::string result = Utils::join(vec, " -> ");
    EXPECT_EQ("100 -> 200", result);
}

TEST_F(UtilsTest, JoinLongStrings)
{
    std::vector<std::string> vec = {
        "very_long_string_one",
        "very_long_string_two",
        "very_long_string_three"};
    std::string result = Utils::join(vec, "|");
    EXPECT_EQ("very_long_string_one|very_long_string_two|very_long_string_three", result);
}

// ==================== COMBINED SPLIT AND JOIN TESTS ====================

TEST_F(UtilsTest, SplitThenJoin)
{
    std::string original = "a|b|c|d";
    std::vector<std::string> split_result = Utils::split(original, '|');
    std::string joined_result = Utils::join(split_result, "|");

    EXPECT_EQ(original, joined_result);
}

TEST_F(UtilsTest, SplitJoinWithNumbers)
{
    std::string original = "100|200|300";
    std::vector<std::string> split_result = Utils::split(original, '|');
    std::string joined_result = Utils::join(split_result, "|");

    EXPECT_EQ(original, joined_result);
}

TEST_F(UtilsTest, SplitJoinRoundTrip)
{
    std::string original = "one,two,three,four,five";
    std::vector<std::string> parts = Utils::split(original, ',');
    std::string reconstructed = Utils::join(parts, ",");

    EXPECT_EQ(original, reconstructed);
}

// ==================== EDGE CASE TESTS ====================

TEST_F(UtilsTest, SplitOnlyDelimiters)
{
    std::vector<std::string> result = Utils::split("|||", '|');
    EXPECT_EQ(3, result.size());
    for (const auto &s : result)
    {
        EXPECT_EQ("", s);
    }
}

TEST_F(UtilsTest, JoinWithEmptyStrings)
{
    std::vector<std::string> vec = {"", "", ""};
    std::string result = Utils::join(vec, "|");
    EXPECT_EQ("||", result);
}

TEST_F(UtilsTest, SplitWithSpecialCharacters)
{
    std::vector<std::string> result = Utils::split("192.168.1.0/24|10.0.0.0/8", '|');
    ASSERT_EQ(2, result.size());
    EXPECT_EQ("192.168.1.0/24", result[0]);
    EXPECT_EQ("10.0.0.0/8", result[1]);
}
