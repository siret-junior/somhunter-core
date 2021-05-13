#include <string>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>

#include "CanvasQueryRanker.h"

using namespace std;

// // ***
// // Tests for:
// /* void add_flag(
//         char short_name,
//         const std::string& long_name,
//         const std::string& help = "",
//         bool required = false) {} */

// #define SUITE_NAME add_flag__basic__short_long

// TEST(SUITE_NAME, required_char_flags_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag('a', "aa", "...", false);
//     p.add_flag('b', "bb", "...", false);
//     p.add_flag('c', "cc", "...", true);
//     p.add_flag('d', "dd", "...", true);

//     vector<string> input{ "program", "-c", "--dd" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_FALSE(p.has('a'));
//     EXPECT_FALSE(p.has("aa"));

//     EXPECT_FALSE(p.has('b'));
//     EXPECT_FALSE(p.has("bb"));

//     EXPECT_TRUE(p.has('c'));
//     EXPECT_TRUE(p.has("cc"));

//     EXPECT_TRUE(p.has('d'));
//     EXPECT_TRUE(p.has("dd"));
// };

// TEST(SUITE_NAME, optional_flags_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag('a', "aa", "...", false);
//     p.add_flag('b', "bb", "...", false);

//     vector<string> input{ "program", "-a" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('a'));
//     EXPECT_TRUE(p.has("aa"));

//     EXPECT_FALSE(p.has('b'));
//     EXPECT_FALSE(p.has("bb"));
// };

// TEST(SUITE_NAME, multiple_same_optional_flags_work) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag('a', "aa", "...", false);
//     p.add_flag('b', "bb", "...", false);

//     vector<string> input{ "program", "-a", "-a", "--aa" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('a'));
//     EXPECT_TRUE(p.has("aa"));

//     EXPECT_FALSE(p.has('b'));
//     EXPECT_FALSE(p.has("bb"));
// };

// TEST(SUITE_NAME, multiple_same_required_flags_work) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag('c', "cc", "...", true);
//     p.add_flag('d', "dd", "...", true);

//     vector<string> input{ "program", "--cc", "-c", "--cc" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('c'));
//     EXPECT_TRUE(p.has("cc"));

//     EXPECT_FALSE(p.has('d'));
//     EXPECT_FALSE(p.has("dd"));
// }

// TEST(SUITE_NAME, missing_required_flag_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag('c', "cc", "...", true);
//     p.add_flag('d', "dd", "...", false);

//     vector<string> input{ "program", "--dd" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// /* void add_flag(
//         const std::string& long_name,
//         const std::string& help = "",
//         bool required = false) {} */

// #define SUITE_NAME add_flag__basic__long

// TEST(SUITE_NAME, required_char_flags_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_flag("bb", "...", false);
//     p.add_flag("cc", "...", true);
//     p.add_flag("dd", "...", true);

//     vector<string> input{ "program", "--cc", "--dd" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_FALSE(p.has("aa"));
//     EXPECT_FALSE(p.has("bb"));

//     EXPECT_TRUE(p.has("cc"));
//     EXPECT_TRUE(p.has("dd"));
// };

// TEST(SUITE_NAME, optional_flags_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_flag("bb", "...", false);

//     vector<string> input{ "program", "--aa" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("aa"));
//     EXPECT_FALSE(p.has("bb"));
// };

// TEST(SUITE_NAME, multiple_same_optional_flags_work) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_flag("bb", "...", false);

//     vector<string> input{ "program", "-aa", "-aa", "--aa" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("aa"));
//     EXPECT_FALSE(p.has("bb"));
// };

// TEST(SUITE_NAME, multiple_same_required_flags_work) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("cc", "...", true);
//     p.add_flag("dd", "...", true);

//     vector<string> input{ "program", "--cc", "-cc", "--cc" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("cc"));
//     EXPECT_FALSE(p.has("dd"));
// }

// TEST(SUITE_NAME, missing_required_flag_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("cc", "...", true);
//     p.add_flag("dd", "...", false);

//     vector<string> input{ "program", "--dd" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// /*  template <typename T, typename = cmd_parser_supported_t<T>>
//     void add_opt(
//         char short_name,
//         const std::string& long_name,
//         const std::string& help = "",
//         bool required = false) {} */

// #define SUITE_NAME add_opt__basic__short_long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", true);
//     p.add_opt<int>('i', "int", "...", true);
//     p.add_opt<float>('f', "float", "...", true);
//     p.add_opt<double>('d', "double", "...", true);
//     p.add_opt<string>('s', "string", "...", true);

//     p.add_opt<string>('o', "opotional", "...", false);
    

//     vector<string> input{ "program", "--bool=true", "-i", "10", "-f3.3", "--double=3.4", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('b'));
//     EXPECT_TRUE(p.has("bool"));
//     EXPECT_EQ(p.get_opt<bool>('b'), true);
//     EXPECT_EQ(p.get_opt<bool>("bool"), true);

//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>('i'), 10);
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - 3.3)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 3.3)), 0.0001);

//     EXPECT_TRUE(p.has('d'));
//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - 3.4)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 3.4)), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>('s'), string{"hello"});
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});

//     EXPECT_FALSE(p.has('o'));
//     EXPECT_FALSE(p.has("optional"));
// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", false);
//     p.add_opt<int>('i', "int", "...", false);
//     p.add_opt<float>('f', "float", "...", false);
//     p.add_opt<double>('d', "double", "...", false);
//     p.add_opt<string>('s', "string", "...", false);

//     p.add_opt<string>('o', "optional", "...", false);
    

//     vector<string> input{ "program", "--bool=true", "-i", "10", "-f3.3", "--double=3.4", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('b'));
//     EXPECT_TRUE(p.has("bool"));
//     EXPECT_EQ(p.get_opt<bool>('b'), true);
//     EXPECT_EQ(p.get_opt<bool>("bool"), true);

//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>('i'), 10);
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - 3.3)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 3.3)), 0.0001);

//     EXPECT_TRUE(p.has('d'));
//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - 3.4)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 3.4)), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>('s'), string{"hello"});
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});

//     EXPECT_TRUE(p.has('o'));
//     EXPECT_TRUE(p.has("optional"));
// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", true);
//     p.add_opt<int>('i', "int", "...", false);

//     vector<string> input{ "program", "--int" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// /* template <typename T, typename = cmd_parser_supported_t<T>>
//     void add_opt(
//         const std::string& long_name,
//         const std::string& help = "",
//         bool required = false) {} */

// #define SUITE_NAME add_opt__basic__long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     p.add_opt<string>("opotional", "...", false);
    

//     vector<string> input{ "program", "--bool=true", "--int=10", "--float=3.3", "--double=3.4", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("bool"));
//     EXPECT_EQ(p.get_opt<bool>("bool"), true);

//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 3.3)), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 3.4)), 0.0001);

//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});

//     EXPECT_FALSE(p.has("optional"));
// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>("bool", "...", false);
//     p.add_opt<int>("int", "...", false);
//     p.add_opt<float>("float", "...", false);
//     p.add_opt<double>("double", "...", false);
//     p.add_opt<string>("string", "...", false);

//     p.add_opt<string>("opotional", "...", false);
    

//     vector<string> input{ "program", "--bool=true", "--int=10", "--float=3.3", "--double=3.4", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("bool"));
//     EXPECT_EQ(p.get_opt<bool>("bool"), true);

//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 3.3)), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 3.4)), 0.0001);

//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});

//     EXPECT_FALSE(p.has("optional"));
// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", false);

//     vector<string> input{ "program", "--int" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// /* template <typename T, typename = cmd_parser_supported_non_bool_t<T>>
//     void add_opt(
//         char short_name,
//         const std::string& long_name,
//         const std::string& help,
//         const std::vector<T>& choices,
//         bool required = false) {} */

// #define SUITE_NAME add_opt__enum__short_long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", { 10, 11 }, true);
//     p.add_opt<float>('f', "float", "...", { 1.1F, -2.2F }, true);
//     p.add_opt<double>('d', "double", "...", { 1.1, -2.2 },  true);
//     p.add_opt<string>('s', "string", "...", { "hello", "world" },  true);
    

//     vector<string> input{ "program", "-i", "10", "-f-2.2", "--double=1.1", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>('i'), 10);
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - (-2.2) )), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - (-2.2))), 0.0001);

//     EXPECT_TRUE(p.has('d'));
//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - 1.1)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 1.1)), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>('s'), string{"hello"});
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});

// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", { 10, 11 }, false);
//     p.add_opt<float>('f', "float", "...", { 1.1F, -2.2F }, false);
//     p.add_opt<double>('d', "double", "...", { 1.1, -2.2 },  false);
//     p.add_opt<string>('s', "string", "...", { "hello", "world" },  false);
    

//     vector<string> input{ "program", "-i", "10", "-f-2.2", "--double=1.1", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>('i'), 10);
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - (-2.2) )), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - (-2.2))), 0.0001);

//     EXPECT_TRUE(p.has('d'));
//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - 1.1)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 1.1)), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>('s'), string{"hello"});
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});
// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", { 10, 11 }, true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// /* template <typename T, typename = cmd_parser_supported_non_bool_t<T>>
//     void add_opt(
//         const std::string& long_name,
//         const std::string& help,
//         const std::vector<T>& choices,
//         bool required = false) {} */


// #define SUITE_NAME add_opt__enum__long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>("int", "...", { 10, 11 }, true);
//     p.add_opt<float>("float", "...", { 1.1F, -2.2F }, true);
//     p.add_opt<double>("double", "...", { 1.1, -2.2 },  true);
//     p.add_opt<string>("string", "...", { "hello", "world" },  true);
    

//     vector<string> input{ "program", "--int=10", "--float=-2.2", "--double=1.1", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - (-2.2))), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 1.1)), 0.0001);

//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});

// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>("int", "...", { 10, 11 }, false);
//     p.add_opt<float>("float", "...", { 1.1F, -2.2F }, false);
//     p.add_opt<double>("double", "...", { 1.1, -2.2 },  false);
//     p.add_opt<string>("string", "...", { "hello", "world" },  false);
    

//     vector<string> input{ "program", "--int=10", "--float=-2.2", "--double=1.1", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - (-2.2))), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 1.1)), 0.0001);

//     EXPECT_TRUE(p.has("string"));
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});
// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", { 10, 11 }, true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// /* template <typename T, typename = is_numeric_t<T>>
//     void add_opt(
//         const std::string& long_name,
//         const std::string& help,
//         T upper_bound,
//         T lower_bound,
//         bool required = false) {} */


// #define SUITE_NAME add_opt__bounded__long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>("int", "...", 10, 11, true);
//     p.add_opt<float>("float", "...", -1.0, 2.0F, true);
//     p.add_opt<double>("double", "...", -1.0, 2.0,  true);
   
//     vector<string> input{ "program", "--int=10", "--float=0.2", "--double=-0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 0.2)), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - (-0.1) )), 0.0001);

// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>("int", "...", 10, 11, false);
//     p.add_opt<float>("float", "...", -1.0, 2.0F, false);
//     p.add_opt<double>("double", "...", -1.0, 2.0,  false);
   
//     vector<string> input{ "program", "--int=10", "--float=0.2", "--double=-0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 0.2)), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - (-0.1) )), 0.0001);
// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", 10, 11, true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // can be int, float, double
// // bounds are inclusive
// /* template <typename T, typename = is_numeric_t<T>>
//     void add_opt(
//         char short_name,
//         const std::string& long_name,
//         const std::string& help,
//         T upper_bound,
//         T lower_bound,
//         bool required = false) {} */

// #define SUITE_NAME add_opt__bounded__short_long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", 10, 11, true);
//     p.add_opt<float>('f', "float", "...", -1.0, 2.0F, true);
//     p.add_opt<double>('d', "double", "...", -1.0, 2.0,  true);
   
//     vector<string> input{ "program", "-i10", "-f", "0.2", "-d", "0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_TRUE(p.has('i'));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);
//     EXPECT_EQ(p.get_opt<int>('i'), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_TRUE(p.has('f'));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - 0.2)), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has('d'));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - (-0.1) )), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - (-0.1) )), 0.0001);

// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", 10, 11, false);
//     p.add_opt<float>('f', "float", "...", -1.0, 2.0F, false);
//     p.add_opt<double>('d', "double", "...", -1.0, 2.0,  false);
   
//     vector<string> input{ "program", "-i10", "-f", "0.2", "-d", "0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_TRUE(p.has('i'));
//     EXPECT_EQ(p.get_opt<int>("int"), 10);
//     EXPECT_EQ(p.get_opt<int>('i'), 10);

//     EXPECT_TRUE(p.has("float"));
//     EXPECT_TRUE(p.has('f'));
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - 0.2)), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has('d'));
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - (-0.1) )), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - (-0.1) )), 0.0001);
// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<int>('i', "int", "...", 10, 11, true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // !!! I expect the items in the same order as in the input
// /* template <typename T, typename = cmd_parser_supported_t<T>>
//     void add_vector_opt(
//         char short_name,
//         const std::string& long_name,
//         const std::string& help = "",
//         bool required = false) {} */

// #define SUITE_NAME add_vector_opt__basic__short_long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program", "-btrue,false,true", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('b'));
//     EXPECT_TRUE(p.has("bool"));
//     ASSERT_EQ(p.get_vector_opt<bool>('b').size(), 3);
//     ASSERT_EQ(p.get_vector_opt<bool>("bool").size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[2], true);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[2], true);


//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>('i').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[1], -8);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>('f').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>('f')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has('d'));
//     ASSERT_EQ(p.get_vector_opt<double>('d').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>('d')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     EXPECT_TRUE(p.has("string"));
//     ASSERT_EQ(p.get_vector_opt<string>('s').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<string>("string").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<string>('s')[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>('s')[1], "world");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[1], "world");

// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", false);
//     p.add_vector_opt<int>('i', "int", "...", false);
//     p.add_vector_opt<float>('f', "float", "...", false);
//     p.add_vector_opt<double>('d', "double", "...", false);
//     p.add_vector_opt<string>('s', "string", "...", false);
   
//     vector<string> input{ "program", "-btrue,false,true", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('b'));
//     EXPECT_TRUE(p.has("bool"));
//     ASSERT_EQ(p.get_vector_opt<bool>('b').size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[2], true);
//     ASSERT_EQ(p.get_vector_opt<bool>("bool").size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[2], true);

//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>('i').size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[1], -8);
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>('f').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>('f')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has('d'));
//     ASSERT_EQ(p.get_vector_opt<double>('d').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>('d')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     EXPECT_TRUE(p.has("string"));
//     ASSERT_EQ(p.get_vector_opt<string>('s').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<string>("string").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<string>('s')[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>('s')[1], "world");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[1], "world");

// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>('i', "int", "...", true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // verze bez short
// // new function
// // delimiter is hard coded to be ','
// /* template <typename T, typename = cmd_parser_supported_t<T>>
//     void add_vector_opt(
//         const std::string& long_name,
//         const std::string& help = "",
//         bool required = false) {} */

// #define SUITE_NAME add_vector_opt__basic__long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>("bool", "...", true);
//     p.add_vector_opt<int>("int", "...", true);
//     p.add_vector_opt<float>("float", "...", true);
//     p.add_vector_opt<double>("double", "...", true);
//     p.add_vector_opt<string>("string", "...", true);
   
//     vector<string> input{ "program", "--bool=true,false,true", "--int=10,-8", "--float=0.2,-0.1", "--double=0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("bool"));
//     ASSERT_EQ(p.get_vector_opt<bool>("bool").size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[2], true);

//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("string"));
//     ASSERT_EQ(p.get_vector_opt<string>("string").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<string>("string")[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[1], "world");

// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>("bool", "...", false);
//     p.add_vector_opt<int>("int", "...", false);
//     p.add_vector_opt<float>("float", "...", false);
//     p.add_vector_opt<double>("double", "...", false);
//     p.add_vector_opt<string>("string", "...", false);
   
//     vector<string> input{ "program", "--bool=true,false,true", "--int=10,-8", "--float=0.2,-0.1", "--double=0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("bool"));
//     ASSERT_EQ(p.get_vector_opt<bool>("bool").size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[2], true);

//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("string"));
//     ASSERT_EQ(p.get_vector_opt<string>("string").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<string>("string")[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[1], "world");

// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // delimiter is hard coded to be ','
// // bounds inclusive
// /* template <typename T, typename = is_numeric_t<T>>
//     void add_vector_opt(
//         char short_name,
//         const std::string& long_name,
//         const std::string& help,
//         T upper_bound,
//         T lower_bound,
//         bool required = false) {} */

// #define SUITE_NAME add_vector_opt__bounded__short_long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>('i', "int", "...", -10, 10, true);
//     p.add_vector_opt<float>('f', "float", "...", -20.0F, 10.0F, true);
//     p.add_vector_opt<double>('d', "double", "...", -20.0, 10.0, true);
   
//     vector<string> input{ "program", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert

//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>('i').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[1], -8);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>('f').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>('f')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has('d'));
//     ASSERT_EQ(p.get_vector_opt<double>('d').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>('d')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);


// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>('i', "int", "...", -10, 10, false);
//     p.add_vector_opt<float>('f', "float", "...", -20.0F, 10.0F, false);
//     p.add_vector_opt<double>('d', "double", "...", -20.0, 10.0, false);
   
//     vector<string> input{ "program", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert

//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>('i').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[1], -8);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>('f').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>('f')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has('d'));
//     ASSERT_EQ(p.get_vector_opt<double>('d').size(), 2);
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>('d')[0] - 0.2)), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>('i', "int", "...", 10, 2000, true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // delimiter is hard coded to be ','
// // bounds inclusive
// /* template <typename T, typename = is_numeric_t<T>>
//     void add_vector_opt(
//         const std::string& long_name,
//         const std::string& help,
//         T upper_bound,
//         T lower_bound,
//         bool required = false) {} */

// #define SUITE_NAME add_vector_opt__bounded__long

// TEST(SUITE_NAME, required_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", -10, 10, true);
//     p.add_vector_opt<float>("float", "...", -20.0F, 10.0F, true);
//     p.add_vector_opt<double>("double", "...", -20.0, 10.0, true);
   
//     vector<string> input{ "program", "--int=10,-8", "--float=0.2,-0.1", "--double=0.2,-0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert

//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);


// }

// TEST(SUITE_NAME, optional_option_parsed) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", -10, 10, false);
//     p.add_vector_opt<float>("float", "...", -20.0F, 10.0F, false);
//     p.add_vector_opt<double>("double", "...", -20.0, 10.0, false);
   
//     vector<string> input{ "program", "--int=10,-8", "--float=0.2,-0.1", "--double=0.2,-0.1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert

//     EXPECT_TRUE(p.has("int"));
//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     EXPECT_TRUE(p.has("float"));
//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     EXPECT_TRUE(p.has("double"));
//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

// }

// TEST(SUITE_NAME, missing_required_opt_throws) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", 10, 2000, true);

//     vector<string> input{ "program" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // can be bool, int, float, double, string
// // returns the value of command line argument 'arg_name' if present, else throws an exception
// /*
// template <typename T, typename = cmd_parser_supported_t<T>>
// T get_opt(const std::string& long_name) { return T{}; } */

// #define SUITE_NAME get_opt__long

// TEST(SUITE_NAME, gets_the_correct_values) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", true);
//     p.add_opt<int>('i', "int", "...", true);
//     p.add_opt<float>('f', "float", "...", true);
//     p.add_opt<double>('d', "double", "...", true);
//     p.add_opt<string>('s', "string", "...",  true);
    

//     vector<string> input{ "program", "-btrue", "-i", "10", "-f-2.2", "--double=1.1", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_EQ(p.get_opt<bool>("bool"), 10);
//     EXPECT_EQ(p.get_opt<int>("int"), 10);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>("float") - (-2.2))), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>("double") - 1.1)), 0.0001);
//     EXPECT_EQ(p.get_opt<string>("string"), string{"hello"});
// }

// TEST(SUITE_NAME, throw_on_missing) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", true);
//     p.add_opt<int>('i', "int", "...", true);
//     p.add_opt<float>('f', "float", "...", true);
//     p.add_opt<double>('d', "double", "...", true);
//     p.add_opt<string>('s', "string", "...",  true);
    

//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_ANY_THROW(p.get_opt<bool>("bool"));
//     EXPECT_ANY_THROW(p.get_opt<int>("int"));
//     EXPECT_ANY_THROW(p.get_opt<float>("float"));
//     EXPECT_ANY_THROW(p.get_opt<double>("double"));
//     EXPECT_ANY_THROW(p.get_opt<string>("string"));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // can be bool, int, float, double, string
// // returns the value of command line argument 'arg_name' if present, else throws an exception
// //template <typename T, typename = cmd_parser_supported_t<T>>
// //T get_opt(char short_name) { return T{}; }

// #define SUITE_NAME get_opt__short

// TEST(SUITE_NAME, gets_the_correct_values) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", true);
//     p.add_opt<int>('i', "int", "...", true);
//     p.add_opt<float>('f', "float", "...", true);
//     p.add_opt<double>('d', "double", "...", true);
//     p.add_opt<string>('s', "string", "...",  true);
    

//     vector<string> input{ "program", "-btrue", "-i", "10", "-f-2.2", "--double=1.1", "--string=hello" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_EQ(p.get_opt<bool>('b'), true);
//     EXPECT_EQ(p.get_opt<int>('i'), 10);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_opt<float>('f') - (-2.2) )), 0.0001);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_opt<double>('d') - 1.1)), 0.0001);
//     EXPECT_EQ(p.get_opt<string>('s'), string{"hello"});
// }

// TEST(SUITE_NAME, throw_on_missing) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_opt<bool>('b', "bool", "...", true);
//     p.add_opt<int>('i', "int", "...", true);
//     p.add_opt<float>('f', "float", "...", true);
//     p.add_opt<double>('d', "double", "...", true);
//     p.add_opt<string>('s', "string", "...",  true);
    

//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_ANY_THROW(p.get_opt<bool>('b'));
//     EXPECT_ANY_THROW(p.get_opt<int>('i'));
//     EXPECT_ANY_THROW(p.get_opt<float>('f'));
//     EXPECT_ANY_THROW(p.get_opt<double>('d'));
//     EXPECT_ANY_THROW(p.get_opt<string>('s'));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // can be bool, int, float, double, string
// // returns the vector of values of command line argument 'long_name' if present, else throws an exception
// //template <typename T, typename = cmd_parser_supported_t<T>>
// //const std::vector<T>& get_vector_opt(const std::string& long_name) { return std::vector<T>{}; }

// #define SUITE_NAME get_vector_opt__long

// TEST(SUITE_NAME, gets_the_correct_values) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program", "-btrue,false,true", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     ASSERT_EQ(p.get_vector_opt<bool>("bool").size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>("bool")[2], true);

//     ASSERT_EQ(p.get_vector_opt<int>("int").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>("int")[1], -8);

//     ASSERT_EQ(p.get_vector_opt<float>("float").size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>("float")[1] - -(0.1) )), 0.0001);

//     ASSERT_EQ(p.get_vector_opt<double>("double").size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>("double")[1] - -(0.1) )), 0.0001);

//     ASSERT_EQ(p.get_vector_opt<string>("string").size(), 2);
//     EXPECT_EQ(p.get_vector_opt<string>("string")[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>("string")[1], "world");
// }

// TEST(SUITE_NAME, throw_on_missing) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
    
//     EXPECT_ANY_THROW(p.get_vector_opt<bool>("bool"));
//     EXPECT_ANY_THROW(p.get_vector_opt<int>("int"));
//     EXPECT_ANY_THROW(p.get_vector_opt<float>("float"));
//     EXPECT_ANY_THROW(p.get_vector_opt<double>("double"));
//     EXPECT_ANY_THROW(p.get_vector_opt<string>("string"));
// }

// #undef SUITE_NAME // get_vector_opt__long

// // ***
// // Tests for:
// // can be bool, int, float, double, string
// // returns the vector of values of command line argument 'short_name' if present, else throws an exception
// //template <typename T, typename = cmd_parser_supported_t<T>>
// //const std::vector<T>& get_vector_opt(char short_name) { return std::vector<T>{}; }

// #define SUITE_NAME get_vector_opt__short

// TEST(SUITE_NAME, gets_the_correct_values) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program", "-btrue,false,true", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('b'));
//     ASSERT_EQ(p.get_vector_opt<bool>('b').size(), 3);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[0], true);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[1], false);
//     EXPECT_EQ(p.get_vector_opt<bool>('b')[2], true);

//     EXPECT_TRUE(p.has('i'));
//     ASSERT_EQ(p.get_vector_opt<int>('i').size(), 2);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[0], 10);
//     EXPECT_EQ(p.get_vector_opt<int>('i')[1], -8);

//     EXPECT_TRUE(p.has('f'));
//     ASSERT_EQ(p.get_vector_opt<float>('f').size(), 2);
//     EXPECT_LT(static_cast<float>(std::fabs(p.get_vector_opt<float>('f')[0] - 0.2)), 0.0001);

//     EXPECT_TRUE(p.has('d'));
//     ASSERT_EQ(p.get_vector_opt<double>('d').size(), 2);
//     EXPECT_LT(static_cast<double>(std::fabs(p.get_vector_opt<double>('d')[0] - 0.2)), 0.0001);

//     EXPECT_TRUE(p.has('s'));
//     ASSERT_EQ(p.get_vector_opt<string>('s').size(), 2);
//     EXPECT_EQ(p.get_vector_opt<string>('s')[0], "hello");
//     EXPECT_EQ(p.get_vector_opt<string>('s')[1], "world");
// }

// TEST(SUITE_NAME, throw_on_missing) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_ANY_THROW(p.get_vector_opt<bool>('b'));
//     EXPECT_ANY_THROW(p.get_vector_opt<int>('i'));
//     EXPECT_ANY_THROW(p.get_vector_opt<float>('f'));
//     EXPECT_ANY_THROW(p.get_vector_opt<double>('d'));
//     EXPECT_ANY_THROW(p.get_vector_opt<string>('s'));
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// // can be bool, int, float, double, string
// // true if arg_name was specified in argv
// // bool has(const std::string& long_name);

// #define SUITE_NAME has__long

// TEST(SUITE_NAME, correctly_reports_presence) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program", "-btrue,false,true", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has("bool"));
//     EXPECT_TRUE(p.has("int"));
//     EXPECT_TRUE(p.has("float"));
//     EXPECT_TRUE(p.has("double"));
//     EXPECT_TRUE(p.has("string"));
// }

// TEST(SUITE_NAME, correctly_reports_missing) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_FALSE(p.has("bool"));
//     EXPECT_FALSE(p.has("int"));
//     EXPECT_FALSE(p.has("float"));
//     EXPECT_FALSE(p.has("double"));
//     EXPECT_FALSE(p.has("string"));
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// // can be bool, int, float, double, string
// // true if arg_name was specified in argv
// // bool has(char short_name);

// #define SUITE_NAME has__short

// TEST(SUITE_NAME, correctly_reports_presence) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program", "-btrue,false,true", "-i10,-8", "-f", "0.2,-0.1", "-d", "0.2,-0.1", "--string=hello,world" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_TRUE(p.has('b'));
//     EXPECT_TRUE(p.has('i'));
//     EXPECT_TRUE(p.has('f'));
//     EXPECT_TRUE(p.has('d'));
//     EXPECT_TRUE(p.has('s'));
// }

// TEST(SUITE_NAME, correctly_reports_missing) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<bool>('b', "bool", "...", true);
//     p.add_vector_opt<int>('i', "int", "...", true);
//     p.add_vector_opt<float>('f', "float", "...", true);
//     p.add_vector_opt<double>('d', "double", "...", true);
//     p.add_vector_opt<string>('s', "string", "...", true);
   
//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     EXPECT_FALSE(p.has('b'));
//     EXPECT_FALSE(p.has('i'));
//     EXPECT_FALSE(p.has('f'));
//     EXPECT_FALSE(p.has('d'));
//     EXPECT_FALSE(p.has('s'));
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // !!! This throws on validation errors
// // parses arguments by given rules
// // void parse_args(int argc, const char* argv[]);

// #define SUITE_NAME parse_args__argc_argv

// TEST(SUITE_NAME, does_not_throw_on_valid) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", 10, 2000, true);

//     const char* input[] = { "program", "--int=10,500" };

//     int argc{ 2 };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_NO_THROW(p.parse_args(argc, input));
// }

// TEST(SUITE_NAME, throws_on_invalid) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", 10, 2000, true);

//     const char* input[] = { "program", "--int=10,500000" };

//     int argc{ 2 };


//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(argc, input));
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// // !!! This throws on validation errors
// // parses arguments by given rules
// // void parse_args(std::vector<std::string>& args);

// #define SUITE_NAME parse_args__vector_strings

// TEST(SUITE_NAME, does_not_throw_on_valid) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", 10, 2000, true);

//     vector<string> input{ "program", "--int=10,500" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_NO_THROW(p.parse_args(input));
// }

// TEST(SUITE_NAME, throws_on_invalid) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_vector_opt<int>("int", "...", 10, 2000, true);

//     vector<string> input{ "program", "--int=-10,50000" };

//     // ***
//     // Act & Assert
//     // \todo Make this more specific (`EXPECT_THROW(stmt, exception)`).
//     EXPECT_ANY_THROW(p.parse_args(input));
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// // returns plain command line arguments (specified after two dashes)
// //const std::vector<std::string>& plain_args();

// #define SUITE_NAME plain_args

// TEST(SUITE_NAME, correctly_returns_empty_args) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--double", "--bool" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto args{ p.plain_args() };
//     EXPECT_EQ(args.size(), 0);
// }

// TEST(SUITE_NAME, correctly_returns_nonempty_args) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--bool", "--double", "--", "ARG1", "ARG2" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto args{ p.plain_args() };
//     EXPECT_EQ(args.size(), 2);
    
//     EXPECT_EQ(args[0], "ARG1");
//     EXPECT_EQ(args[1], "ARG2");
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // returns the size of plain command line arguments
// // size_t plain_args_count();

// #define SUITE_NAME plain_args_count

// TEST(SUITE_NAME, correct_empty_count) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--double", "--aa" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto args{ p.plain_args() };
//     EXPECT_EQ(args.size(), 0);
// }

// TEST(SUITE_NAME, correct_nonempty_count) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--string", "--float", "--", "ARG1", "ARG2" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto args{ p.plain_args() };
//     EXPECT_EQ(args.size(), 2);
// }

// #undef SUITE_NAME


// // ***
// // Tests for:
// // new function
// // returns help string with all set command line arguments and their types
// // const std::string& help_str();

// #define SUITE_NAME help_str

// TEST(SUITE_NAME, returns_correct_string) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--bool", "--aa" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert

//     string expected_output{ "..." };
//     string act_str{p.help_str()};

//     EXPECT_GT(act_str.length(), 0);
//     // \todo This needs the actual implementation to check.
//     //EXPECT_EQ(act_st, expected_output);
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // new function
// // returns the number of parsed options
// // size_t parsed_opts_count() const;

// #define SUITE_NAME parsed_opts_count

// TEST(SUITE_NAME, correct_number_returned_on_empty) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto count{p.parsed_opts_count()};
//     EXPECT_GT(count, 0);
// }

// TEST(SUITE_NAME, correct_number_returned_on_nonempty) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--aa", "--bool", "--", "ARG1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto count{p.parsed_opts_count()};
//     EXPECT_GT(count, 3);
// }

// #undef SUITE_NAME

// // ***
// // Tests for:
// // new function
// // returns vector of long names of parsed options
// // const std::vector<std::string>& parsed_opts() const;

// #define SUITE_NAME parsed_opts

// TEST(SUITE_NAME, correct_names_returned_on_empty) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto opt_names{p.parsed_opts()};
//     EXPECT_EQ(opt_names.size(), 0);
// }

// TEST(SUITE_NAME, correct_names_returned_on_nonempty) {
//     // ***
//     // Arrange
//     parser p;
//     p.add_flag("aa", "...", false);
//     p.add_opt<bool>("bool", "...", true);
//     p.add_opt<int>("int", "...", true);
//     p.add_opt<float>("float", "...", true);
//     p.add_opt<double>("double", "...", true);
//     p.add_opt<string>("string", "...", true);

//     vector<string> input{ "program", "--float=3.3", "-b10", "--", "ARG1" };

//     // ***
//     // Act
//     p.parse_args(input);

//     // ***
//     // Assert
//     auto opt_names{p.parsed_opts()};
//     EXPECT_EQ(opt_names.size(), 2);
//     EXPECT_EQ(opt_names[0], string{ "float" });
//     EXPECT_EQ(opt_names[1], string{ "bool" });
// }
// 
// #undef SUITE_NAME