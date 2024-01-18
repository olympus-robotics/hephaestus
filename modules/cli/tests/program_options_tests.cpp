//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "eolo/base/exception.h"
#include "eolo/cli/program_options.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace eolo::cli::tests {

TEST(ProgramOptions, Empty) {
  auto desc = ProgramDescription("A dummy service that does nothing");
  auto options = std::move(desc).parse({});

  EXPECT_TRUE(options.hasOption("help"));
}

TEST(ProgramOptions, Option) {
  static constexpr float NUMBER = 1.1f;
  auto desc = ProgramDescription("A dummy service that does nothing");
  desc.defineOption<std::string>("option", "desc")
      .defineOption<std::string>("other_option", 'o', "other desc")
      .defineOption<float>("bar", "desc", NUMBER)
      .defineOption<int>("foo", 'f', "desc", 1);
  {
    auto options = ProgramDescription{ desc }.parse(
        { "--option", "value", "-o", "other_value", "--bar", "1.2" });
    EXPECT_TRUE(options.hasOption("option"));
    EXPECT_EQ(options.getOption<std::string>("option"), "value");
    EXPECT_TRUE(options.hasOption("other_option"));
    EXPECT_EQ(options.getOption<std::string>("other_option"), "other_value");
    EXPECT_TRUE(options.hasOption("bar"));
    EXPECT_EQ(options.getOption<float>("bar"), 1.2f);
    EXPECT_TRUE(options.hasOption("foo"));
    EXPECT_EQ(options.getOption<int>("foo"), 1);
  }
}

TEST(ProgramOptions, Errors) {
  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    EXPECT_THROW(std::move(desc).parse({ "--option" }), InvalidParameterException);
  }

  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    desc.defineOption<std::string>("option", "desc").defineOption<int>("other", "desc");
    EXPECT_THROW(ProgramDescription{ desc }.parse({}), InvalidConfigurationException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option" }), InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "value" }), InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option", "--other_option" }),
                 InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option", "value", "other_value" }),
                 InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option", "value" }),
                 InvalidConfigurationException);
  }

  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    desc.defineOption<int>("option", "desc");
    auto options = ProgramDescription{ desc }.parse({ "--option", "1.2" });
    auto v = options.getOption<int>("option");
    EXPECT_EQ(v, 1);
    // EXPECT_THROW(std::ignore = options.getOption<int>("option"), TypeMismatchException);
  }
}

}  // namespace eolo::cli::tests
