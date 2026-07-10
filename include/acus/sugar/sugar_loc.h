// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#define LOC loc
#define LOC_FWD LOC
#define SUGAR_LOC std::source_location LOC
#define SUGAR_FUNC SUGAR_LOC = std::source_location::current()
