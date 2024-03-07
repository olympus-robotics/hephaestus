//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

namespace heph::cli {
/// @return true if key was pressed on the terminal console. use getch() to read the key stroke.
auto kbhit() -> bool;

/// @return Code for the last keypress from the terminal console without blocking
auto getch() -> int;
}  // namespace heph::cli
