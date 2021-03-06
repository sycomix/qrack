cmake_minimum_required (VERSION 3.9)
project (QRack VERSION 1.0 DESCRIPTION "High Performance Quantum Bit Simulation")

# Installation commands
include (GNUInstallDirs)

set (OPENCL_AMDSDK /opt/AMDAPPSDK-3.0 CACHE PATH "Installation path for the installed AMD OpenCL SDK, if used")
option (ENABLE_OPENCL "Use OpenCL optimizations" ON)

# Options used when building the project
find_library (LIB_OPENCL OpenCL)
if (NOT LIB_OPENCL)
    # Attempt with AMD's OpenCL SDK
    find_library (LIB_OPENCL OpenCL PATHS ${OPENCL_AMDSDK}/lib/x86_64/)
    if (NOT LIB_OPENCL)
        set (ENABLE_OPENCL OFF)
    else ()
        # Found, set the required include path.
        set (OPENCL_INCLUDE_PATH ${OPENCL_AMDSDK}/include CACHE PATH "AMD OpenCL SDK Header include path")
        set (OPENCL_COMPILATION_OPTIONS
            -Wno-ignored-attributes
            -Wno-deprecated-declarations
            CACHE STRING "AMD OpenCL SDK Compilation Option Requirements")
        message ("OpenCL support found in the AMD SDK")
    endif ()
endif ()

message ("OpenCL Support is: ${ENABLE_OPENCL}")

if (ENABLE_OPENCL)
    message ("    libOpenCL: ${LIB_OPENCL}")
    message ("    Includes:  ${OPENCL_INCLUDE_PATH}")
    message ("    Options:   ${OPENCL_COMPILATION_OPTIONS}")
endif ()

include_directories ("include" "include/common")

# Declare the library
add_library (qrack STATIC
    src/common/complex16simd.cpp
    src/common/parallel_for.cpp
    src/qengine/qengine.cpp
    src/qengine/operators.cpp
    src/qengine/gates.cpp
    src/qengine/rotational.cpp
    src/qengine/state/state.cpp
    src/qengine/state/operators.cpp
    src/qengine/state/gates.cpp
    # src/qunit.cpp
    )

# Declare the unittest executable
add_executable (unittest
    test/test_main.cpp
    test/tests.cpp
    )

target_link_libraries (unittest
    qrack
    pthread
    )

add_test (NAME qrack_tests
    COMMAND unittest
    )

enable_testing()

# Run the unittest executable on 'make test'

target_include_directories (unittest PUBLIC test)

target_compile_options (qrack PUBLIC -ggdb -std=c++11 -Wall -Werror -DCATCH_CONFIG_FAST_COMPILE)
target_compile_options (unittest PUBLIC -ggdb -std=c++11 -Wall -Werror -DCATCH_CONFIG_FAST_COMPILE)

if (ENABLE_OPENCL)
    target_compile_definitions (qrack PUBLIC ENABLE_OPENCL=1)

    # Include the necessary options and libraries to link against
    target_include_directories (qrack PUBLIC ${PROJECT_BINARY_DIR} ${OPENCL_INCLUDE_PATH})
    target_compile_options (qrack PUBLIC ${OPENCL_COMPILATION_OPTIONS})
    target_link_libraries (unittest ${LIB_OPENCL})

    # Build the OpenCL command files
    find_program (XXD_BIN xxd)
    file (GLOB_RECURSE COMPILED_RESOURCES "src/qengine/state/*.cl")
    foreach (INPUT_FILE ${COMPILED_RESOURCES})
        get_filename_component (INPUT_NAME ${INPUT_FILE} NAME)
        get_filename_component (INPUT_BASENAME ${INPUT_FILE} NAME_WE)
        get_filename_component (INPUT_DIR ${INPUT_FILE} DIRECTORY)

        set (OUTPUT_FILE ${PROJECT_BINARY_DIR}/${INPUT_BASENAME}cl.hpp)

        message (" Creating XXD Rule for ${INPUT_FILE} -> ${OUTPUT_FILE}")
        add_custom_command (
            WORKING_DIRECTORY ${INPUT_DIR}
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${XXD_BIN} -i ${INPUT_NAME} > ${OUTPUT_FILE}
            COMMENT "Building OpenCL Commands in ${INPUT_FILE}"
            )
        list (APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
    endforeach ()

    # Add the OpenCL objects to the library
    target_sources (qrack PRIVATE
        ${COMPILED_RESOURCES}
        src/common/oclengine.cpp
        src/qengine/state/opencl.cpp
        )

else (ENABLE_OPENCL)
    target_compile_definitions (qrack PUBLIC ENABLE_OPENCL=0)
endif (ENABLE_OPENCL)

set_target_properties (qrack PROPERTIES
    VERSION ${PROJECT_VERSION}
    )

# Install common headers
install (FILES
    include/common/complex16simd.hpp
    include/common/oclengine.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qrack/common
    )

# Install qrack library headers
install (FILES 
    include/qengine_cpu.hpp
    include/qunit.hpp
    include/common/oclengine.hpp
    include/common/complex16simd.hpp
    include/common/parallel_for.hpp
    include/qengine_opencl.hpp
    include/qinterface.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qrack
    )

# Install the archive
install (TARGETS qrack
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

# Install the pkg-config file
configure_file (qrack.pc.in qrack.pc @ONLY)
install (FILES ${CMAKE_BINARY_DIR}/qrack.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

