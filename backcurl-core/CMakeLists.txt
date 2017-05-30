cmake_minimum_required(VERSION 2.8.7)

# Allow use of project folders for IDEs like Visual Studio, so we
# could organize projects into relevant folders: "cpr", "tests" & "external (libraries)".
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

set(PROJ backcurl)
project(backcurl)

if(NOT ${CMAKE_VERSION} LESS 3.2)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    message(STATUS "Checking compiler flags for C++11 support.")
    # Set C++11 support flags for various compilers
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
    check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        message(STATUS "C++11 is supported.")
        if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        endif()
    elseif(COMPILER_SUPPORTS_CXX0X)
        message(STATUS "C++0x is supported.")
        if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        endif()
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()

IF (NOT DEFINED CMAKE_INSTALL_BINDIR)
SET(CMAKE_INSTALL_BINDIR /usr/local/bin)
ENDIF (NOT DEFINED CMAKE_INSTALL_BINDIR)

IF (NOT DEFINED CMAKE_INSTALL_LIBDIR)
SET(CMAKE_INSTALL_LIBDIR /usr/local/lib)
ENDIF (NOT DEFINED CMAKE_INSTALL_LIBDIR)

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include/backcurl)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

MESSAGE(STATUS "Current LIBRARY DIRECTORY located at ${CMAKE_INSTALL_LIBDIR}")
MESSAGE(STATUS "Current SOURCES DIRECTORY located at ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(STATUS "Current Binary DIRECTORY located at ${CMAKE_CURRENT_BINARY_DIR}")

SET(LIBPATH /usr/lib /usr/local/lib)

include_directories(header /usr/local/include)

find_library(CURL_LIB NAMES curl PATHS ${LIBPATH})
if(CURL_LIB)
    message(STATUS "curl found. curl support is compiled in.")
else(CURL_LIB)
    message( FATAL_ERROR "curl not found, curl is not supporting, please install curl")
endif(CURL_LIB)


if(UAPPLE AND NOT UNIX)
add_definitions(-DCSIF_MACOSX=true)
endif()

file(GLOB SRCS src/*.cpp)
file(GLOB HDRS header/*.h)

# SET (SRCS BackCurl.cpp)
# SET (HDRS BackCurl.h)

# add_executable(${PROJ} ${HDRS} ${SRCS})

add_library(${PROJ}.static STATIC ${HDRS} ${SRCS})
set_target_properties(${PROJ}.static PROPERTIES OUTPUT_NAME ${PROJ})
add_library(${PROJ} SHARED ${HDRS} ${SRCS})

link_directories(/usr/local/lib /usr/lib)


target_link_libraries(${PROJ} curl pthread)

install(TARGETS ${PROJ} EXPORT ${PROJ}
        ARCHIVE DESTINATION lib${LIB_SUFFIX}
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
install(TARGETS ${PROJ}.static EXPORT ${PROJ}
        ARCHIVE DESTINATION lib${LIB_SUFFIX}
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

install(FILES header/BackCurl.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(CODE "execute_process(COMMAND ldconfig)")
