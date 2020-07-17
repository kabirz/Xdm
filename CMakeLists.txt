cmake_minimum_required(VERSION 3.8)

project(Xdm)

find_package(X11)
if(X11_FOUND)
	include_directories(${X11_INCLUDE_DIR})
else()
	message(FATAL_ERROR "Cannot find X11 package")
endif()

find_package(Freetype)
if(FREETYPE_FOUND)
	include_directories(${FREETYPE_INCLUDE_DIR_freetype2})
else()
	message(FATAL_ERROR "Cannot find Freetype package")
endif()

add_compile_options(-std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os)
add_definitions(-D_DEFAULT_SOURCE -D_BSD_SOURCE)
add_link_options(-s)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
######## build dwm ########
set(DWM_VERSION "6.2")
add_executable(dwm
	dwm/drw.c
	dwm/dwm.c
	dwm/util.c
)
set_target_properties(dwm PROPERTIES COMPILE_DEFINITIONS VERSION=\"${DWM_VERSION}\")
target_link_libraries(dwm ${X11_X11_LIB} ${X11_Xft_LIB} ${Fontconfig_LIBRARIES})


######## build st ########
add_executable(st
	st/st.c
	st/x.c
)
set(ST_VERION "0.8.4")
set_target_properties(st PROPERTIES COMPILE_DEFINITIONS "VERSION=\"${ST_VERSION}\";_XOPEN_SOURCE=600")
target_link_libraries(st ${X11_X11_LIB} ${X11_Xft_LIB} ${Fontconfig_LIBRARIES} -lutil)
