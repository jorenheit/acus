add_library(acus STATIC
  api/api.cc
  assembler/assembler_access.cc
  assembler/assembler_add.cc
  assembler/assembler_algorithms.cc
  assembler/assembler_binop_general.cc
  assembler/assembler_blocks.cc
  assembler/assembler_builders.cc
  assembler/assembler_cache.cc
  assembler/assembler_calls_control.cc
  assembler/assembler_codegen.cc
  assembler/assembler_comparisons.cc
  assembler/assembler_diag.cc
  assembler/assembler_divmod.cc
  assembler/assembler_framenav.cc
  assembler/assembler_globals.cc
  assembler/assembler_logical.cc
  assembler/assembler_memory.cc
  assembler/assembler_mul.cc
  assembler/assembler_pointers_memory.cc
  assembler/assembler_print.cc
  assembler/assembler_program.cc
  assembler/assembler_rlvalue.cc
  assembler/assembler_sub.cc
  assembler/assembler_unary.cc
  core/proxy.cc
  core/proxy_arrayelement.cc
  core/proxy_derefptr.cc
  core/proxy_direct.cc
  core/proxy_globalreference.cc
  core/proxy_structfield.cc
  core/slot.cc
  ir/constants.cc
  ir/primitive.cc
  sugar/sugar_api.cc
  sugar/sugar_expr.cc
  sugar/sugar_impl.cc
  sugar/sugar_io.cc
  types/literal.cc
  types/literal_builders.cc
  types/literal_impl.cc
  types/operators.cc
  types/type_builders.cc
  types/type_rules.cc
  types/typesystem.cc
)

add_library(Acus::acus ALIAS acus)

target_compile_features(acus PUBLIC cxx_std_23)
set_target_properties(acus PROPERTIES
  CXX_EXTENSIONS OFF
  OUTPUT_NAME acus
)

acus_enable_warnings(acus)

target_include_directories(acus
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
  TARGETS acus
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Install both the compatibility header <acus.h> and the namespaced public
# headers such as <acus/acus.h> and <acus/sugar/sugar.h>.
install(
  FILES "${PROJECT_SOURCE_DIR}/include/acus.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/include/acus/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/acus"
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hh"
    PATTERN "*.hpp"
    PATTERN "*.hxx"
    PATTERN "*.ih"
    PATTERN "*.tpp"
    PATTERN "*.ipp"
)
