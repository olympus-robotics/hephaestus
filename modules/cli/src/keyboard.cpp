//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/cli/keyboard.h"

#include <cstdio>
#include <ctime>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace heph::cli {

auto kbhit() -> bool {
  /// reference:
  /// https://github.com/cvilas/grape/blob/d008b53816272a0860487464fb12fa4d837759ad/modules/common/conio/src/conio.cpp#L17
  timeval tv{};  // NOLINT(misc-include-cleaner)
  fd_set fds{};
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration,cppcoreguidelines-pro-bounds-constant-array-index)
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);  // NOLINT(hicpp-signed-bitwise)
  select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
  return FD_ISSET(STDIN_FILENO, &fds);  // NOLINT(hicpp-signed-bitwise)
}

auto getch() -> int {
  /// reference:
  /// https://github.com/cvilas/grape/blob/d008b53816272a0860487464fb12fa4d837759ad/modules/common/conio/src/conio.cpp#L31
  struct termios oldt = {};
  tcgetattr(STDIN_FILENO, &oldt);  // grab old terminal io settings
  struct termios newt = oldt;
  // disable buffered io
  newt.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));  // NOLINT(hicpp-signed-bitwise)
  tcsetattr(STDERR_FILENO, TCSANOW, &newt);                 // new settings set
  const int ch = getchar();
  tcsetattr(STDERR_FILENO, TCSANOW, &oldt);  // terminal settings restored
  return ch;
}

}  // namespace heph::cli
