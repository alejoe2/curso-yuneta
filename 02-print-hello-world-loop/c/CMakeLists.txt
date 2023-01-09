cmake_minimum_required(VERSION 3.0)

project(02HelloWorldProjectLoop C)

set(CMAKE_INSTALL_PREFIX /CursoYuneta)

set(BIN_DEST_DIR ${CMAKE_INSTALL_PREFIX}/bin)

add_executable(02HelloWorldLoop src/main.c)

install(
	TARGETS 02HelloWorldLoop
	PERMISSIONS
	OWNER_READ OWNER_WRITE
	GROUP_READ GROUP_WRITE
	WORLD_READ
	DESTINATION ${BIN_DEST_DIR}
)

