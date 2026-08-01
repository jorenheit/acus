file(GLOB ACUS_EXAMPLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

foreach(source IN LISTS ACUS_EXAMPLE_SOURCES)
  get_filename_component(example_name "${source}" NAME_WE)
  set(target_name "acus-example-${example_name}")

  add_executable("${target_name}" "${source}")
  target_compile_features("${target_name}" PRIVATE cxx_std_23)
  set_target_properties("${target_name}" PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "${example_name}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/examples"
  )
  acus_enable_warnings("${target_name}")
  target_link_libraries("${target_name}" PRIVATE Acus::acus)
endforeach()
