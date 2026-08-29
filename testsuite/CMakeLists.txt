	add_executable(acus-runtime-tests testsuite.cc)
target_compile_features(acus-runtime-tests PRIVATE cxx_std_23)
set_target_properties(acus-runtime-tests PROPERTIES CXX_EXTENSIONS OFF)
acus_enable_warnings(acus-runtime-tests)
target_link_libraries(acus-runtime-tests PRIVATE Acus::acus)

add_executable(acus-compiletime-tests compiletime_testsuite.cc)
target_compile_features(acus-compiletime-tests PRIVATE cxx_std_23)
set_target_properties(acus-compiletime-tests PROPERTIES CXX_EXTENSIONS OFF)
acus_enable_warnings(acus-compiletime-tests)
target_link_libraries(acus-compiletime-tests PRIVATE Acus::acus)


if(MSVC)
  target_compile_options(acus-runtime-tests PRIVATE /Od)
  target_compile_options(acus-compiletime-tests PRIVATE /Od)
else()
  target_compile_options(acus-runtime-tests PRIVATE -O0)
  target_compile_options(acus-compiletime-tests PRIVATE -O0)
endif()


add_test(NAME acus-runtime COMMAND acus-runtime-tests)
add_test(NAME acus-compiletime COMMAND acus-compiletime-tests)
