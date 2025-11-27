// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#pragma once

#include <random>

#include <gtest/gtest.h>

#include "hephaestus/error_handling/panic_as_exception_scope.h"
#include "hephaestus/utils/stack_trace.h"

namespace heph::test_utils {

class HephTest : public ::testing::Test {
public:
  HephTest();

  HephTest(const HephTest&) = delete;
  HephTest(HephTest&&) = delete;

  auto operator=(const HephTest&) -> HephTest& = delete;
  auto operator=(HephTest&&) -> HephTest& = delete;

  ~HephTest() override;

  std::mt19937_64 mt;  // NOLINT(misc-non-private-member-variables-in-classes,
                       // cppcoreguidelines-non-private-member-variables-in-classes)

private:
  const utils::StackTrace trace_;
  const error_handling::PanicAsExceptionScope panic_scope_;
};

}  // namespace heph::test_utils
