#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
  spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  spdlog::set_level(spdlog::level::debug);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
