#pragma once

// Println implementation
void acus::sugar::println(auto&& arg, SUGAR_LOC) {
  print(std::forward<decltype(arg)>(arg), LOC_FWD);
  print('\n', LOC_FWD);
}
