if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build --verbose)
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --release --verbose)
    set(TARGET_DIR "release")
endif ()

if(ENABLE_LTO)
    set(RUST_FLAGS "-Clinker-plugin-lto" "-Clinker=clang" "-Clink-arg=-fuse-ld=lld")
endif()

set(RUST_RUSTY_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/librust_rusty.a")

set(RUST_RUSTY_CXX "${CMAKE_CURRENT_BINARY_DIR}/rust_rusty.cpp")
add_library(rust_rusty STATIC ${RUST_RUSTY_CXX})
add_custom_command(
    OUTPUT ${RUST_RUSTY_CXX}
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} RUSTFLAGS="${RUST_FLAGS}" ${CARGO_CMD}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/rust_rusty/src/lib.rs.cc ${RUST_RUSTY_CXX}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/rust_rusty/src/lib.rs.h ${CMAKE_CURRENT_BINARY_DIR}/rust_rusty.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rust_rusty pthread dl ${RUST_RUSTY_LIB})

add_test(NAME rust_rusty_test 
    COMMAND cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

