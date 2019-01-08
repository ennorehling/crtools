cmake_minimum_required(VERSION 2.9)
project(crtools C)

find_package(PNG)

if (MSVC)
find_package (PDCurses)
SET(CURSES_FOUND ${PDCURSES_FOUND})
SET(CURSES_LIBRARIES ${PDCURSES_LIBRARY})
SET(CURSES_INCLUDE_DIR ${PDCURSES_INCLUDE_DIR})
else (MSVC)
find_package (Curses)
endif (MSVC)

add_library(crtools
	crparse.c
	hierarchy.c
	conversion.c
	command.c
	crdata.c
	origin.c)

if (CURSES_FOUND)
include_directories (${CURSES_INCLUDE_DIR})
add_executable(eva eva.c evadata.c)
target_link_libraries(eva crtools)
target_link_libraries(eva ${CURSES_LIBRARIES})
endif(CURSES_FOUND)

