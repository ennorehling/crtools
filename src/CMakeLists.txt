cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 99)

set (SHARED_SRC
  crparse.c 
  hierarchy.c
  conversion.c
  command.c 
  crdata.c
  origin.c 
) 

if (MSVC)
  find_package (PDCurses)
  set (CURSES_FOUND ${PDCURSES_FOUND})
  set (CURSES_LIBRARIES ${PDCURSES_LIBRARIES})
  set (CURSES_INCLUDE_DIRS ${PDCURSES_INCLUDE_DIR})
endif(MSVC)

find_package(LibXml2)
#find_package (PNG)
find_package (Iconv)

if (NOT CURSES_FOUND)
  set(CURSES_NEED_WIDE TRUE)
  set(CURSES_NEED_NCURSES TRUE)
  find_package (Curses)
  if (NOT CURSES_FOUND)
    set(CURSES_NEED_WIDE FALSE)
    find_package (Curses REQUIRED)
  endif(NOT CURSES_FOUND)
endif (NOT CURSES_FOUND)

add_library(crtools ${SHARED_SRC})

add_executable(eformat eformat.c)
target_link_libraries(eformat crtools)

# if (ICONV_FOUND)
# if (LIBXML2_FOUND)
# add_executable(cr2xml cr2xml.c)
# target_include_directories (cr2xml PRIVATE ${ICONV_INCLUDE_DIRS})
# target_include_directories (cr2xml PRIVATE ${LiBXML2_INCLUDE_DIRS})
# target_link_libraries(cr2xml
#   crtools
#   ${ICONV_LIBRARIES}
#   ${LIBXML2_LIBRARIES})
# endif (LIBXML2_FOUND)
# endif (ICONV_FOUND)

add_executable(cr2html cr2html.c)
target_link_libraries(cr2html crtools)

add_executable(crmerian crmerian.c)
target_link_libraries(crmerian crtools)
add_executable(crmerge crmerge.c)
target_link_libraries(crmerge crtools)
add_executable(crstrip crstrip.c)
target_link_libraries(crstrip crtools)
add_executable(crcutter crcutter.c)
target_link_libraries(crcutter crtools)

if (PNG_FOUND)
add_executable(crimage crimage.c image.c)
target_link_libraries(crimage crtools)
target_include_directories (crimage PRIVATE ${PNG_INCLUDE_DIRS})
target_link_libraries(crimage crtools ${PNG_LIBRARIES})
endif (PNG_FOUND)

if (CURSES_FOUND)
add_executable(eva eva.c evadata.c)
target_include_directories (eva PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(eva crtools ${CURSES_LIBRARIES})
endif (CURSES_FOUND)
