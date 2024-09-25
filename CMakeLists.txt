cmake_minimum_required(VERSION 3.15)

project(TelegramQRBot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

file(GLOB_RECURSE TGBOT_SRC CONFIGURE_DEPENDS "src/*.h" "src/*.cpp" "src/*.hpp" "src/*.c")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(CURL)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)

if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

add_executable(${PROJECT_NAME} ${TGBOT_SRC})

find_library(SQLITE3_LIBRARY sqlite3)
if(NOT SQLITE3_LIBRARY)
    message(FATAL_ERROR "SQLite3 Library not found")
endif()

include_directories(${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${SQLITE3_LIBRARY} ${PostgreSQL_LIBRARIES} nlohmann_json::nlohmann_json)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)


if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE OS_WINDOWS)
    message("Configuring for Windows OS")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_compile_definitions(${PROJECT_NAME} PRIVATE OS_FREEBSD)
    message("Configuring for FreeBSD OS")
else()
    message(FATAL_ERROR "Unsupported OS")
endif()


foreach(SOURCE IN ITEMS ${TGBOT_SRC})
    file(RELATIVE_PATH SOURCE_RELATIVE_PATH "${CMAKE_SOURCE_DIR}/src" "${SOURCE}")
    get_filename_component(SOURCE_PATH "${SOURCE_RELATIVE_PATH}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_MSVC "${SOURCE_PATH}")
    source_group("${SOURCE_PATH_MSVC}" FILES "${SOURCE}")
endforeach()
