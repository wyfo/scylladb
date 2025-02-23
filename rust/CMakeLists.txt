find_program(CARGO cargo)
if(NOT CARGO)
  message(FATAL "cargo is required")
endif()

function(add_rust_library name)
  # use for profiles defined in Cargo.toml
  set(mode ${CMAKE_BUILD_TYPE})
  string(TOLOWER ${CMAKE_BUILD_TYPE} mode)
  set(target_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(profile "rust-${mode}")
  set(library ${target_dir}/${profile}/lib${name}.a)
  add_custom_command(
    OUTPUT ${library}
    COMMAND ${CMAKE_COMMAND} -E env CARGO_BUILD_DEP_INFO_BASEDIR=. ${CARGO} build --locked --target-dir=${target_dir} --profile=rust-${mode}
    DEPENDS Cargo.lock
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Rust library: ${library}")
  add_custom_target(rust-${name}
    DEPENDS ${library})
  add_library(Rust::${name} STATIC IMPORTED)
  add_dependencies(Rust::${name} rust-${name})
  set_target_properties(Rust::${name} PROPERTIES
    IMPORTED_LOCATION "${library}")
endfunction(add_rust_library)

find_program(CXXBRIDGE cxxbridge)
if(NOT CXXBRIDGE)
  message(FATAL "cxxbridge is required")
endif()
# Generate cxxbridge header and source
function(generate_cxxbridge name)
  cmake_parse_arguments(parsed_args
    ""
    "INPUT;OUTPUT_DIR;INCLUDE;SOURCES" "" ${ARGN})
  set(input ${CMAKE_CURRENT_SOURCE_DIR}/${parsed_args_INPUT})
  set(output_dir ${parsed_args_OUTPUT_DIR})
  set(header ${output_dir}/${name}.hh)
  set(source ${output_dir}/${name}.cc)
  set(outputs ${header} ${source})
  set(command ${CXXBRIDGE} ${input} --include ${parsed_args_INCLUDE})
  add_custom_command(
    DEPENDS ${input} ${include}
    OUTPUT ${outputs}
    COMMAND ${command} --header --output ${header}
    COMMAND ${command} --output ${source})

  set(${parsed_args_SOURCES} ${outputs} PARENT_SCOPE)
endfunction(generate_cxxbridge)


add_rust_library(rust_combined)

set(cxx_header "${CMAKE_CURRENT_BINARY_DIR}/cxx.h")
add_custom_command(
  OUTPUT ${cxx_header}
  COMMAND ${CXXBRIDGE} --header --output ${cxx_header})
generate_cxxbridge(wasmtime_bindings
  INPUT wasmtime_bindings/src/lib.rs
  INCLUDE ${cxx_header}
  OUTPUT_DIR "${scylla_gen_build_dir}"
  SOURCES wasmtime_bindings_sources)

set_target_properties(Rust::rust_combined PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}")

add_library(wasmtime_bindings)
target_sources(wasmtime_bindings
  PRIVATE
    ${cxx_header}
    ${wasmtime_bindings_sources})
target_include_directories(wasmtime_bindings
  INTERFACE
    ${CMAKE_BINARY_DIR})
target_link_libraries(wasmtime_bindings
  INTERFACE Rust::rust_combined)
