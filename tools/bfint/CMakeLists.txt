set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

add_executable(bfint
  main.cc
  bfint.cc
)

target_compile_features(bfint PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(bfint PRIVATE /O2)
else()
  target_compile_options(bfint PRIVATE -O3)
endif()

set_target_properties(bfint PROPERTIES CXX_EXTENSIONS OFF)
acus_enable_warnings(bfint)

target_include_directories(bfint PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(bfint PRIVATE ${CURSES_LIBRARIES})

install(
  TARGETS bfint
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
