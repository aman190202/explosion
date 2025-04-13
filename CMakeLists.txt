cmake_minimum_required(VERSION 3.12)
project(volume_renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set OpenMP paths for macOS
if(APPLE)
    set(OpenMP_C_FLAGS "-Xclang -fopenmp")
    set(OpenMP_CXX_FLAGS "-Xclang -fopenmp")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
endif()

# Find OpenMP
find_package(OpenMP REQUIRED)

# Include directories
include_directories(
    /opt/homebrew/include
    /opt/homebrew/include/openvdb
    /opt/homebrew/include/openvdb/include
    /opt/homebrew/opt/libomp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Eigen
    /opt/homebrew/Cellar/openvdb
    /opt/homebrew/Cellar/openvdb/include
)

list(APPEND CMAKE_MODULE_PATH "/usr/local/lib/cmake/OpenVDB")

find_package(OpenVDB "12.0.1" REQUIRED COMPONENTS openvdb nanovdb)

# Link directories
link_directories(
    /opt/homebrew/lib
    /opt/homebrew/opt/libomp/lib
)

# Create executables
add_executable(volume_render volume_render.cpp)
add_executable(analyze_vdb analyze_vdb.cpp)
add_executable(vdb_to_nvdb vdb_to_nvdb.cpp)
add_executable(nvdb_to_vdb nvdb_to_vdb.cpp)

# Link libraries
target_link_libraries(volume_render 
    openvdb
    tbb
    boost_system
    Iex-3_3
    IlmThread-3_3
    OpenEXR-3_3
    z
    OpenMP::OpenMP_CXX
)

target_link_libraries(analyze_vdb
    openvdb
    tbb
    boost_system
    Iex-3_3
    IlmThread-3_3
    OpenEXR-3_3
    z
)

target_compile_definitions(vdb_to_nvdb PRIVATE "NANOVDB_USE_OPENVDB")
target_link_libraries(vdb_to_nvdb OpenVDB::openvdb OpenVDB::nanovdb)
target_compile_definitions(nvdb_to_vdb PRIVATE "NANOVDB_USE_OPENVDB")
target_link_libraries(nvdb_to_vdb OpenVDB::openvdb OpenVDB::nanovdb)
