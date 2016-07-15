INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/res)

SET(PREFIX "${CMAKE_INSTALL_PREFIX}")
SET(RES_DIR "${PREFIX}/res")
SET(LOCALE_DIR "${RES_DIR}/locale")
SET(SHARED_DIR "${PREFIX}/shared")
SET(VERSION 0.1.0)

SET(DAEMON_SRCS cbhmd.c
	cbhmd_convert.c
	cbhmd_drawer.c
	cbhmd_handler.c
	cbhmd_item.c
	cbhmd_eldbus.c	
	cbhmd_storage.c
	cbhmd_utils.c

)

IF (WAYLAND_SUPPORT)
SET(DAEMON_SRCS ${DAEMON_SRCS} cbhmd_wl_drawer.c
    cbhmd_wl_handler.c
)
ENDIF (WAYLAND_SUPPORT)

IF (X11_SUPPORT)
SET(DAEMON_SRCS ${DAEMON_SRCS} cbhmd_x_drawer.c
	cbhmd_x_handler.c
)
ENDIF (X11_SUPPORT)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF (MDM_ENABLE)
	 ADD_DEFINITIONS( -DMDM_ENABLE )
	 SET(MDM_REQUIRED_PKGS "mdm")
ENDIF (MDM_ENABLE)

IF (WAYLAND_SUPPORT)
	ADD_DEFINITIONS("-DHAVE_WAYLAND")
ENDIF (WAYLAND_SUPPORT)

IF (X11_SUPPORT)
	ADD_DEFINITIONS("-DHAVE_X11")
ENDIF (X11_SUPPORT)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	elementary
	eet
	appcore-efl
	appcore-common
	eina
	evas
	ecore
	ecore-file
	ecore-evas
	edje ecore-input
	notification
	vconf
	vconf-internal-keys
	libsystemd-daemon
	libtzplatform-config
	dlog
	${MDM_REQUIED_PKGS}
)

IF (WAYLAND_SUPPORT)
pkg_check_modules(pkgs_extra REQUIRED
	ecore-wayland
)
ENDIF (WAYLAND_SUPPORT)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${pkgs_extra_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fvisibility=hidden -Wall -fPIC -g")

SET(CMAKE_CXX_FLAGS_PROFILING  "-O0 -std=c++0x -g -pg -fPIE")
SET(CMAKE_C_FLAGS_DEBUG        "-O0 -g -fPIE")
SET(CMAKE_CXX_FLAGS_DEBUG      "-O0 -std=c++0x -g -fPIE")
SET(CMAKE_C_FLAGS_RELEASE      "-O2 -g -fPIE")
SET(CMAKE_CXX_FLAGS_RELEASE    "-O2 -std=c++0x -g -fPIE")
SET(CMAKE_C_FLAGS_CCOV         "-O2 -g --coverage -fPIE")
SET(CMAKE_CXX_FLAGS_CCOV       "-O2 -std=c++0x -g --coverage -fPIE")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_DEFINITIONS("-DRES_DIR=\"${RES_DIR}\"")
ADD_DEFINITIONS("-DLOCALE_DIR=\"${LOCALE_DIR}\"")
ADD_DEFINITIONS("-DSHARED_DIR=\"${SHARED_DIR}\"")

ADD_EXECUTABLE(${DAEMON} ${DAEMON_SRCS})
#for XV extension screenshot
TARGET_LINK_LIBRARIES(${DAEMON} ${pkgs_LDFLAGS} ${pkgs_extra_LDFLAGS})

ADD_CUSTOM_TARGET(cbhmdrawer.edj
	   COMMAND edje_cc
	   -id ${CMAKE_SOURCE_DIR}/res/images
	   -sd ${CMAKE_SOURCE_DIR}/res/sounds
	   ${CMAKE_SOURCE_DIR}/res/edje/cbhmdrawer.edc ${CMAKE_BINARY_DIR}/res/edje/cbhmdrawer.edj
	   DEPENDS ${CMAKE_SOURCE_DIR}/res/edje/cbhmdrawer.edc
)
ADD_DEPENDENCIES(${DAEMON} cbhmdrawer.edj)
 
INSTALL(TARGETS ${DAEMON} DESTINATION ${PREFIX}/bin)
INSTALL(FILES ${CMAKE_BINARY_DIR}/res/edje/cbhmdrawer.edj DESTINATION ${RES_DIR}/edje)
ADD_SUBDIRECTORY(po)

