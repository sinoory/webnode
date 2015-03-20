# Copyright (C) 2013 Christian Dywan <christian@twotoasts.de>

set(LIBMIDORI_VERSION 0.0.1)
set(LIBMIDORI_SOVERSION 0)
set(EXTRALIB_DIR_secret "/usr/include/libsecret-1")

set(MIDORI_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/BrowserApp")

file(GLOB MIDORI_VALA_SOURCE ../katze/*.vala *.vala ../panels/*.vala)
file(GLOB LIBMIDORI_SOURCE ../katze/*.c ../panels/*.c ../toolbars/*.c  midori-*.vala parse_domain.c midori-*.c sokoke.c Certificate_Password.c Certificate.c password-manager.c ../settings/*.c)
include(GLibHelpers)
add_glib_marshal(LIBMIDORI_SOURCE marshal midori_cclosure_marshal "midori.h")
set(MIDORI_SOURCE
    main.c
    )

include(ValaPrecompile)
vala_precompile(LIBMIDORI_SOURCE_C ${MIDORI_PROJECT_NAME}
    ${MIDORI_VALA_SOURCE}
PACKAGES
    ${PKGS}
OPTIONS
    ${VALAFLAGS}
CUSTOM_VAPIS
    ${EXTRA_VAPIS}
GENERATE_VAPI
    "${MIDORI_PROJECT_NAME}-core"
GENERATE_HEADER
    "${MIDORI_PROJECT_NAME}-core"
)

add_library("${LIBMIDORI}-vala" STATIC ${LIBMIDORI_SOURCE_C})
add_dependencies("${LIBMIDORI}-vala" WebKit2)
target_link_libraries("${LIBMIDORI}-vala"
                      ${DEPS_LIBRARIES}
                      ${OPTS_LIBRARIES}
                      ${DEPS_GTK_LIBRARIES}
                      )
set_target_properties("${LIBMIDORI}-vala" PROPERTIES
                      COMPILE_FLAGS "${VALA_CFLAGS}"
                      POSITION_INDEPENDENT_CODE ON
                      )

add_library(${LIBMIDORI} SHARED ${LIBMIDORI_SOURCE})
add_dependencies(${LIBMIDORI} WebKit2)
if(USE_64BITS)
    target_link_libraries(${LIBMIDORI} "-Wl,--whole-archive ../../../lib/libcrypto.so.1.0.0 -Wl,--no-whole-archive")
    target_link_libraries(${LIBMIDORI} "-Wl,--whole-archive ../../../lib/libssl.so.1.0.0 -Wl,--no-whole-archive")
endif()
include_directories(
                    ${MIDORI_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${EXTRALIB_DIR_secret}
                    "${MIDORI_SOURCE_DIR}/katze"
                    "${MIDORI_SOURCE_DIR}/toolbars"
		    "${MIDORI_SOURCE_DIR}/settings"
                    ${CMAKE_BINARY_DIR}
                    "${CMAKE_SOURCE_DIR}/Source"
                    "${JAVASCRIPTCORE_DIR}"
                    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
                    "${JAVASCRIPTCORE_DIR}/API"
                    "${JAVASCRIPTCORE_DIR}/assembler"
                    "${JAVASCRIPTCORE_DIR}/bytecode"
                    "${JAVASCRIPTCORE_DIR}/bytecompiler"
                    "${JAVASCRIPTCORE_DIR}/dfg"
                    "${JAVASCRIPTCORE_DIR}/disassembler"
                    "${JAVASCRIPTCORE_DIR}/heap"
                    "${JAVASCRIPTCORE_DIR}/debugger"
                    "${JAVASCRIPTCORE_DIR}/interpreter"
                    "${JAVASCRIPTCORE_DIR}/jit"
                    "${JAVASCRIPTCORE_DIR}/llint"
                    "${JAVASCRIPTCORE_DIR}/parser"
                    "${JAVASCRIPTCORE_DIR}/profiler"
                    "${JAVASCRIPTCORE_DIR}/runtime"
                    "${JAVASCRIPTCORE_DIR}/yarr"
                    "${WTF_DIR}"
                    "${DERIVED_SOURCES_DIR}"
                    "${DERIVED_SOURCES_DIR}/ForwardingHeaders"
                    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}"
                    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector"
                    ${DERIVED_SOURCES_WEBKIT2GTK_DIR}
                    ${FORWARDING_HEADERS_WEBKIT2GTK_DIR}
                    ${FORWARDING_HEADERS_DIR}
                    ${GTK3_INCLUDE_DIRS}
                    ${GLIB_INCLUDE_DIRS}
                    ${LIBSOUP_INCLUDE_DIRS}
                    ${FORWARDING_HEADERS_WEBKIT2GTK_EXTENSION_DIR}
                    ${DEPS_INCLUDE_DIRS}
                    ${OPTS_INCLUDE_DIRS}
                    #add openssl path by luyue
                    ${CMAKE_SOURCE_DIR}/Source/ThirdParty/openssl-1.0.0d/include
                    )
target_link_libraries(${LIBMIDORI}
                      "${LIBMIDORI}-vala"
                      JavaScriptCore
                      WebKit2
                      ${GTK3_LIBRARY}
                      ${GDK3_LIBRARY}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      atk-1.0
                      pangocairo-1.0
                      gdk_pixbuf-2.0
                      cairo-gobject
                      pango-1.0
                      ${CAIRO_LIBRARIES}
                      xml2
                      secret-1
                      ${LIBSOUP_LIBRARIES}
                      xslt
                      sqlite3
                      m
                      X11
                      pthread
                      )
set_target_properties(${LIBMIDORI} PROPERTIES
                      COMPILE_FLAGS ${CFLAGS}
                      SOVERSION 1
                      VERSION 0.0.0
                      )

# 默认情况下CMAKE_INSTALL_PREFIX为/usr/local
#install(TARGETS ${LIBMIDORI} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS ${LIBMIDORI} LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${MIDORI_PROJECT_NAME}")

# 若修改了injectedbundle, WebProcess等库位置，则也应修改此处定义
add_definitions(-DWEBKIT_EXEC_PATH="${LIBEXEC_INSTALL_DIR}")
add_definitions(-DWEBKIT_INJECTED_BUNDLE_PATH="${BROWSER_LIB_INSTALL_DIR}")
if (WIN32)
    add_executable(${MIDORI_PROJECT_NAME} WIN32 main.c "${CMAKE_SOURCE_DIR}/data/midori.rc")
else()
    add_executable(${MIDORI_PROJECT_NAME} main.c)
endif()

add_dependencies(${MIDORI_PROJECT_NAME} ${LIBMIDORI})
target_link_libraries(${MIDORI_PROJECT_NAME}
                      ${LIBMIDORI}
                      JavaScriptCore
                      WebKit2
                      ${GTK3_LIBRARY}
                      ${GDK3_LIBRARY}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      atk-1.0
                      pangocairo-1.0
                      gdk_pixbuf-2.0
                      cairo-gobject
                      pango-1.0
                      ${CAIRO_LIBRARIES}
                      xml2
                      secret-1
                      ${LIBSOUP_LIBRARIES}
                      xslt
                      sqlite3
                      m
                      X11
                      pthread
                      )
set_target_properties(${MIDORI_PROJECT_NAME} PROPERTIES
                      COMPILE_FLAGS ${CFLAGS}
                      )
set_target_properties(${MIDORI_PROJECT_NAME} PROPERTIES INSTALL_RPATH "${BROWSER_LIB_INSTALL_DIR}")
install(TARGETS ${MIDORI_PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
