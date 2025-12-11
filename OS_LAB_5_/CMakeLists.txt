cmake_minimum_required(VERSION 3.15)
project(OS_LAB_5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

# Сервер
add_executable(OS_LAB_5
    Server/RecordManager.cpp
    Server/ServerApp.cpp
    Server/OS_LAB_5.cpp
)

target_include_directories(OS_LAB_5 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
)

if(WIN32)
    target_link_libraries(OS_LAB_5 kernel32 user32 advapi32)
endif()

# Клиент
add_executable(Client
    Client/ClientApp.cpp
    Client/PipeClient.cpp
    Client/main.cpp
)

target_include_directories(Client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/Client
)

if(WIN32)
    target_link_libraries(Client kernel32 user32)
endif()

# Тесты
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB TEST_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

add_executable(OS_LAB_5_tests
    ${TEST_SRCS}
    Server/RecordManager.cpp
    Client/PipeClient.cpp
    Client/ClientApp.cpp
    # Note: ServerApp.cpp is *not* required for these unit-tests; 
)

target_include_directories(OS_LAB_5_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
    ${CMAKE_CURRENT_SOURCE_DIR}/Client
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(OS_LAB_5_tests gtest gtest_main)

if(WIN32)
    target_link_libraries(OS_LAB_5_tests kernel32 user32 advapi32)
endif()


include(GoogleTest)
gtest_discover_tests(OS_LAB_5_tests
    TEST_PREFIX "Employee."
    TEST_SUFFIX ""
    PROPERTIES TIMEOUT 30
)


add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --build-config $<CONFIG>
    DEPENDS OS_LAB_5_tests
    COMMENT "Running all unit tests..."
)

add_custom_target(all_and_test
    DEPENDS all_projects run_tests
)


add_custom_target(all_projects DEPENDS OS_LAB_5 Client OS_LAB_5_tests)
