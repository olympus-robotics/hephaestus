//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <limits>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "gmock/gmock.h"
#include "hephaestus/cli/program_options.h"
#include "hephaestus/utils/exception.h"

// NOLINTBEGIN(cert-err58-cpp, cppcoreguidelines-owning-memory, modernize-use-trailing-return-type,
// cppcoreguidelines-avoid-goto) NOLINTNEXTLINE(google-build-using-namespace, modernize-type-traits)
using namespace ::testing;

namespace heph::cli::tests {

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
      .defineOption<int>("foo", 'f', "desc", 1)
      .defineOption<float>("baz", "desc")
      .defineOption<float>("nan_options", "desc", std::numeric_limits<float>::quiet_NaN())
      .defineFlag("flag", 'b', "desc");
  {
    auto options = ProgramDescription{ desc }.parse(
        { "--option", "value", "-o", "other_value", "--bar", "1.2", "--baz", "-1.0" });
    EXPECT_TRUE(options.hasOption("option"));
    EXPECT_EQ(options.getOption<std::string>("option"), "value");
    EXPECT_TRUE(options.hasOption("other_option"));
    EXPECT_EQ(options.getOption<std::string>("other_option"), "other_value");
    EXPECT_TRUE(options.hasOption("bar"));
    EXPECT_EQ(options.getOption<float>("bar"), 1.2f);
    EXPECT_TRUE(options.hasOption("foo"));
    EXPECT_EQ(options.getOption<int>("foo"), 1);
    EXPECT_TRUE(options.hasOption("baz"));
    EXPECT_EQ(options.getOption<float>("baz"), -1.0);
    EXPECT_THAT(options.getOption<float>("nan_options"), IsNan());
    EXPECT_TRUE(options.hasOption("flag"));
    EXPECT_FALSE(options.getOption<bool>("flag"));
  }
  {
    auto options = ProgramDescription{ desc }.parse(
        { "--option", "value", "-o", "other_value", "--bar", "1.2", "--baz", "-20", "--flag" });
    EXPECT_TRUE(options.hasOption("flag"));
    EXPECT_TRUE(options.getOption<bool>("flag"));
  }
}

TEST(ProgramOptions, Errors) {
  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    EXPECT_THROW_OR_DEATH(std::move(desc).parse({ "--option" }), InvalidParameterException,
                          "Undefined option");
  }

  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    desc.defineOption<std::string>("option", "desc").defineOption<int>("other", "desc");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({}), InvalidConfigurationException,
                          "Required option 'option' not specified");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({ "--option" }), InvalidParameterException,
                          "is supposed to be a value");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({ "value" }), InvalidParameterException,
                          "Arg value is not a valid option");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({ "--option", "--other_option" }),
                          InvalidParameterException, "not another option");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({ "--option", "value", "other_value" }),
                          InvalidParameterException, "not a valid option");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({ "--option", "value" }),
                          InvalidConfigurationException, "not specified");
    EXPECT_THROW_OR_DEATH(ProgramDescription{ desc }.parse({ "--option", "-o" }), InvalidParameterException,
                          "not another option");
  }

  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    desc.defineOption<int>("option", "desc");
    auto options = ProgramDescription{ desc }.parse({ "--option", "1.2" });
    auto v = options.getOption<int>("option");
    EXPECT_EQ(v, 1);
  }
}

}  // namespace heph::cli::tests
// NOLINTEND(cert-err58-cpp, cppcoreguidelines-owning-memory, modernize-use-trailing-return-type,
// cppcoreguidelines-avoid-goto, modernize-type-traits)
