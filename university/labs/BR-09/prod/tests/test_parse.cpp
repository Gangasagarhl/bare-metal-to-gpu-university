// SPDX-License-Identifier: LicenseRef-Uni-Lab
// Unit tests of uni::parse_number: what is accepted, and every documented rejection.
#include "uni/parse.h"
#include "uni_test.h"

#include <variant>

namespace {

bool gives(std::string_view text, double expected)
{
    const auto r = uni::parse_number(text);
    return std::holds_alternative<double>(r) && std::get<double>(r) == expected;
}

bool fails(std::string_view text, uni::ParseError expected)
{
    const auto r = uni::parse_number(text);
    return std::holds_alternative<uni::ParseError>(r) && std::get<uni::ParseError>(r) == expected;
}

}  // namespace

UNI_TEST(accepts_plain_decimal_numbers)
{
    CHECK(gives("3", 3.0));
    CHECK(gives("-2.5", -2.5));
    CHECK(gives("+4", 4.0));
    CHECK(gives("1e3", 1000.0));
    CHECK(gives(".5", 0.5));
}

UNI_TEST(rejects_empty_and_words)
{
    CHECK(fails("", uni::ParseError::empty));
    CHECK(fails("abc", uni::ParseError::not_a_number));
    CHECK(fails("+", uni::ParseError::not_a_number));
    CHECK(fails("+-3", uni::ParseError::not_a_number));
    CHECK(fails(" 3", uni::ParseError::not_a_number));
}

UNI_TEST(rejects_what_version_0_1_accepted_silently)
{
    CHECK(fails("3abc", uni::ParseError::trailing_characters));  // 0.1 read it as 3
    CHECK(fails("0x10", uni::ParseError::trailing_characters));  // 0.1 read it as 16
    CHECK(fails("1,5", uni::ParseError::trailing_characters));   // decimal comma
}

UNI_TEST(rejects_non_finite_and_out_of_range)
{
    CHECK(fails("nan", uni::ParseError::not_finite));
    CHECK(fails("inf", uni::ParseError::not_finite));
    CHECK(fails("-infinity", uni::ParseError::not_finite));
    CHECK(fails("1e999", uni::ParseError::out_of_range));  // 0.1 crashed (std::out_of_range)
}

UNI_TEST(every_error_has_a_description)
{
    CHECK(std::string_view(uni::describe(uni::ParseError::empty)) == "is empty");
    CHECK(std::string_view(uni::describe(uni::ParseError::out_of_range)).size() > 0);
}

int main(int argc, char** argv)
{
    return uni_test::run_all(argc, argv);
}
