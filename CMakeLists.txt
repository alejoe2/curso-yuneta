###########################################
#    CMake
###########################################
cmake_minimum_required(VERSION 3.0)

project(CursoYuneta C)

add_subdirectory(01-print-hello-world/c)
add_subdirectory(02-print-hello-world-loop/c)
add_subdirectory(03-print-hello-name/c)
add_subdirectory(04-listen-conecction-tpc/c)
add_subdirectory(05-listen-tpc-file/c)
add_subdirectory(06-yuno-listener/receive_file)

# compile in Release mode optimized but adding debug symbols, useful for profiling :
#
#     cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
#
# or compile with NO optimization and adding debug symbols :
#
#     cmake -DCMAKE_BUILD_TYPE=Debug ..
#
# Compilando para ARM poner cmake -DNOT_INCLUDE_LIBUNWIND
