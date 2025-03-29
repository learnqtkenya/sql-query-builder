cmake_minimum_required(VERSION 3.14)

project(QueryBuilder LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(benchmark CONFIG REQUIRED)

add_executable(QueryBuilder
  main.cpp
)
target_link_libraries(QueryBuilder Qt${QT_VERSION_MAJOR}::Core)

# benchmarks
# add_executable(smallvector_benchmark )
# target_link_libraries(smallvector_benchmark Qt${QT_VERSION_MAJOR}::Core benchmark::benchmark_main)

include(GNUInstallDirs)
install(TARGETS QueryBuilder
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
