#pragma once
#define LOC loc
#define LOC_FWD LOC
#define SUGAR_LOC std::source_location LOC
#define SUGAR_FUNC SUGAR_LOC = std::source_location::current()
