#include <gtest/gtest.h>

#include <wayland/get_option.hpp>

namespace test {

struct get_option : ::testing::Test {};

TEST_F(get_option, returns_nullptr_on_option_absence) {
  char arg0[] = "prog";
  char arg1[] = "-d";
  char arg2[] = "wayland-1";
  char* argv[] = {arg0, arg1, arg2};
  int argc = 3;
  EXPECT_EQ(::get_option(argc, argv, "-s"), nullptr);
}

} // namespace test
