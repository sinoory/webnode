PROJECT(seed)

#pkg-config --modversion gobject-introspection-1.0 |  awk '{ split($1, a , ".") ; printf "0x%02d%02d%02d", a[1], a[2], a[3] }'
add_definitions(-DGOBJECT_INTROSPECTION_VERSION=0x014000)

set(SEED_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Source"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${WTF_DIR}"
    "${GLIB_INCLUDE_DIRS}"
    "/usr/include/gobject-introspection-1.0"
    )

include_directories(${SEED_INCLUDE_DIRECTORIES})

set(SEED_SOURCES
	seed-closure.c 
	seed-api.c 
	seed-closure.h 
	seed-builtins.c 
	seed-builtins.h 
	seed-debug.h 
	seed-exceptions.c 
	seed-exceptions.h 
	seed-engine.c 
	seed-engine.h 
	seed-private.h 
	seed-signals.c 
	seed-signals.h 
	seed-structs.c 
	seed-gtype.c 
	seed-gtype.h 
	seed-structs.h 
	seed-types.c 
	seed-types.h 
	seed-importer.c 
	seed-importer.h 
	seed-path.h
)


ADD_LIBRARY(seed SHARED ${SEED_SOURCES})

target_link_libraries(seed 
    girepository-1.0
    gobject-2.0
    glib-2.0
    gio-2.0
    gmodule-2.0
    JavaScriptCore
    WTF
    )
