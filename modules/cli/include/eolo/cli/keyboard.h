//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

namespace eolo::cli {
/// @return true if key was pressed on the terminal console. use getch() to read the key stroke.
auto kbhit() -> bool;

/// @return Code for the last keypress from the terminal console without blocking
auto getch() -> int;
}  // namespace eolo::cli
