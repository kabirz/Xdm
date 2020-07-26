cmake_minimum_required(VERSION 3.8)

project(Xdm)

option(ENABLE_ST "option for enable ST" OFF)
option(ENABLE_DMENU "option for enable ST" OFF)
set(CMAKE_INSTALL_PREFIX /usr)
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

add_compile_options(-std=c99 -pedantic -Wall -Wno-unused-result -Wno-deprecated-declarations -Os)
add_definitions(-D_DEFAULT_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE)
if (ENABLE_ST)
	add_definitions(-DST)
endif()
if (ENABLE_DMENU)
	add_definitions(-DDMENU)
endif()
add_link_options(-s)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
######## build dwm ########
set(DWM_VERSION "6.2")
add_executable(dwm
	dwm/drw.c
	dwm/dwm.c
	dwm/hardinfo.c
	dwm/util.c
)
set_target_properties(dwm PROPERTIES COMPILE_DEFINITIONS VERSION=\"${DWM_VERSION}\")
target_link_libraries(dwm ${X11_X11_LIB} ${X11_Xft_LIB} ${Fontconfig_LIBRARIES} -lpthread)
install(TARGETS dwm RUNTIME DESTINATION bin)

######## build st ########
if (ENABLE_ST)
add_executable(st
	st/st.c
	st/x.c
)
set(ST_VERION "0.8.4")
set_target_properties(st PROPERTIES COMPILE_DEFINITIONS "VERSION=\"${ST_VERSION}\";_XOPEN_SOURCE=600")
target_link_libraries(st ${X11_X11_LIB} ${X11_Xft_LIB} ${Fontconfig_LIBRARIES} -lutil)
add_custom_command(TARGET st POST_BUILD
                   COMMAND tic -sx st.info
		   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/st
)
install(TARGETS st RUNTIME DESTINATION bin)
endif()
