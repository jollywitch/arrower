#include "catch2/catch.hpp"

#include "arrower/key_codes.hpp"

TEST_CASE("key parser supports generic letter and vk formats") {
    REQUIRE(arrower::ParseVirtualKeyName("A").has_value());
    CHECK(*arrower::ParseVirtualKeyName("A") == 'A');
    REQUIRE(arrower::ParseVirtualKeyName("VK_190").has_value());
    CHECK(*arrower::ParseVirtualKeyName("VK_190") == 190);
}

TEST_CASE("display names stay user-friendly for common punctuation") {
    CHECK(arrower::DisplayVirtualKeyName(0xBE) == ".");
    CHECK(arrower::DisplayVirtualKeyName(0xBF) == "/");
}
