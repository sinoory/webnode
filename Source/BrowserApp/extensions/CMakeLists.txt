# Copyright (C) 2013 Christian Dywan <christian@twotoasts.de>

#set(EXTENSIONDIR "${CMAKE_INSTALL_FULL_LIBDIR}/${MIDORI_PROJECT_NAME}")
set(EXTENSIONDIR "${BROWSER_EXTENSION_INSTALL_DIR}")
set(MIDORI_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/BrowserApp")
set(DERIVED_SOURCE_MIDORI_EXTENSIONS ${DERIVED_SOURCES_MIDORI_DIR}/extensions)

include_directories(
                    "${MIDORI_SOURCE_DIR}"
                    "${MIDORI_SOURCE_DIR}/midori"
                    "${MIDORI_SOURCE_DIR}/katze"
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
                    ${CMAKE_BINARY_DIR}
                    "${CMAKE_BINARY_DIR}/BrowserApp"
                    )

file(GLOB EXTENSIONS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
if (HALF_BRO_INCOM_WEBKIT2)
    list(REMOVE_ITEM EXTENSIONS
         "cookie-permissions"
         "addons.c"
         "formhistory"
         "external-download-manager.vala"
         "nojs"
	 "adblock"
         "nsplugin-manager.vala"
         )
endif ()

# ZRL remove CMake relative files.
list(REMOVE_ITEM EXTENSIONS
      "cmake_install.cmake"
      "Makefile"
      "CMakeLists.txt"
      "CMakeFiles"
      "CTestTestfile.cmake"
      "transfers.h" "transfers_valac.stamp" "transfers.c"
      "apps_valac.stamp" "apps.h" "apps.c"
      "delayed-load_valac.stamp" "delayed-load.c" "delayed-load.h"
      "devpet_valac.stamp" "devpet.h" "devpet.c"
      "open-with_valac.stamp" "open-with.c" "open-with.h"
      "adblock_valac.stamp"
      "tabby_valac.stamp" "tabby.h" "tabby.c"
      "webmedia-now-playing_valac.stamp" "webmedia-now-playing.c" "webmedia-now-playing.h"
      "history-list_valac.stamp" "history-list.h" "history-list.c"
      "domain-keys_valac.stamp" "domain-keys.h" "domain-keys.c"
      "notes_valac.stamp" "notes.c" "notes.h"
      "flummi_valac.stamp" "flummi.c" "flummi.h"
      "about_valac.stamp" "about.h" "about.c" "CMakeDirectoryInformation.cmake"
)

# FIXME: re-enable webmedia extension
# once we have working notifications on win
if (WIN32)
    list(REMOVE_ITEM EXTENSIONS "webmedia-now-playing.vala")
endif()

list(REMOVE_ITEM EXTENSIONS "about.vala" "builtin-extensions-init.vala" 
                 "builtin-extensions-init.h" "builtin-extensions-init.c" "builtin-extensions-init_valac.stamp"
                 "apps.h" "apps.c" "apps.vala" "apps_valac.stamp"
                 "flummi.vala" "flummi.c" "flummi.h" "flummi_valac.stamp"
                 "open-with.vala" "open-with.c" "open-with.h" "open-with_valac.stamp"
                 "tabby.vala" "tabby.c" "tabby.h" "tabby_valac.stamp"
                 "delayed-load.vala" "delayed-load.c" "delayed-load.h" "delayed-load_valac.stamp")

# ZRL 暂时去除以下扩展，待有用时再酌情添加
list(REMOVE_ITEM EXTENSIONS "colorful-tabs.c" "copy-tabs.c" "devpet.vala" "devpet.c" "devpet.h" "devpet_valac.stamp"
                            "domain-keys.vala" "domain-keys.c" "domain-keys.h" "domain-keys_valac.stamp" "cookie-manager"
                            "status-clock.c" "history-list.vala" "history-list.c" "history-list.h" "history-list_valac.stamp"
                            "transfers.vala" "transfers.c" "transfers.h" "weather_inquiry.c"
                            "mouse-gestures.c" "notes.vala" "notes.c" "notes.h" "notes_valac.stamp" "shortcuts.c"
                            "statusbar-features.c" "tab-panel.c" "tab-panel.h" "tabs-minimized.c" "toolbar-editor.c"
                            "webmedia-now-playing.vala" "webmedia-now-playing.c" "webmedia-now-playing.h" "webmedia-now-playing_valac.stamp")
foreach(UNIT_SRC ${EXTENSIONS})
    string(FIND ${UNIT_SRC} ".c" UNIT_EXTENSION)
    if (UNIT_EXTENSION GREATER -1)
        string(REPLACE ".c" "" UNIT ${UNIT_SRC})
        add_library(${UNIT} MODULE ${UNIT_SRC})
        add_dependencies(${UNIT} ${LIBMIDORI})
        target_link_libraries(${UNIT}
                              ${LIBMIDORI}
                              )
        set_target_properties(${UNIT} PROPERTIES
                              COMPILE_FLAGS ${CFLAGS}
                              )
        install(TARGETS ${UNIT}
            LIBRARY DESTINATION ${EXTENSIONDIR}
                )
    endif ()
endforeach ()

foreach(UNIT_SRC ${EXTENSIONS})
    string(FIND ${UNIT_SRC} "." UNIT_EXTENSION)
    if (UNIT_EXTENSION EQUAL -1)
        file(GLOB UNIT_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${UNIT_SRC}/*.c")
        file(GLOB UNIT_FILES_VALA RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${UNIT_SRC}/*.vala")
        if (UNIT_FILES_VALA)
            include(ValaPrecompile)
            vala_precompile(UNIT_SRC_C ${UNIT_SRC}
                ${UNIT_FILES_VALA}
            PACKAGES
                ${PKGS}
            OPTIONS
                ${VALAFLAGS}
            GENERATE_HEADER
                ${UNIT}
            CUSTOM_VAPIS
                ${EXTRA_VAPIS}
                "${MIDORI_SOURCE_DIR}/midori/${MIDORI_PROJECT_NAME}.vapi"
                "${MIDORI_SOURCE_DIR}/katze/katze.vapi"
                "${MIDORI_SOURCE_DIR}/midori/${LIBMIDORI}.vapi"
            )
            set(UNIT_FILES ${UNIT_FILES} ${UNIT_SRC_C})
        endif ()
        if (UNIT_FILES)
            add_library(${UNIT_SRC} MODULE ${UNIT_FILES})
            add_dependencies(${UNIT_SRC} ${LIBMIDORI})
            target_link_libraries(${UNIT_SRC}
                                  ${LIBMIDORI}
                                  )
            install(TARGETS ${UNIT_SRC}
                    LIBRARY DESTINATION ${EXTENSIONDIR}
                    )
            # extensions with vala code get the lenient VALA_CFLAGS
            # others get the usual CFLAGS with -Wall and -Werror
	    if (UNIT_FILES_VALA)
		set_target_properties(${UNIT_SRC} PROPERTIES
				      COMPILE_FLAGS ${VALA_CFLAGS}
				      )
	    else ()
		set_target_properties(${UNIT_SRC} PROPERTIES
				      COMPILE_FLAGS ${CFLAGS}
				      )
	    endif ()
        endif ()
    endif ()
endforeach ()

foreach(UNIT_SRC ${EXTENSIONS})
    string(FIND ${UNIT_SRC} ".vala" UNIT_EXTENSION)
    if (UNIT_EXTENSION GREATER -1)
        string(REPLACE ".vala" "" UNIT ${UNIT_SRC})
        include(ValaPrecompile)
        vala_precompile(UNIT_SRC_C ${UNIT}
            ${UNIT_SRC}
        PACKAGES
            ${PKGS}
        OPTIONS
            ${VALAFLAGS}
        GENERATE_HEADER
            ${UNIT}
        CUSTOM_VAPIS
            ${EXTRA_VAPIS}
            "${MIDORI_SOURCE_DIR}/midori/${MIDORI_PROJECT_NAME}.vapi"
            "${MIDORI_SOURCE_DIR}/katze/katze.vapi"
            "${MIDORI_SOURCE_DIR}/midori/${LIBMIDORI}.vapi"
        )
        add_library(${UNIT} MODULE ${UNIT_SRC_C})
        add_dependencies(${UNIT} ${LIBMIDORI})
        target_link_libraries(${UNIT}
                              ${LIBMIDORI}
                              )
        set_target_properties(${UNIT} PROPERTIES
                              COMPILE_FLAGS "${VALA_CFLAGS}"
                              )
        install(TARGETS ${UNIT}
                LIBRARY DESTINATION ${EXTENSIONDIR}
                )
    endif ()
endforeach ()

include(ValaPrecompile)
set(BUILTIN_EXTENSION_VALA_FILES builtin-extensions-init.vala about.vala delayed-load.vala apps.vala flummi.vala open-with.vala tabby.vala)
set(BUILTIN_EXTENSION_LIBRARY "builtinextension")

include(ValaPrecompile)
vala_precompile(BUILTIN_EXTENSION_SOURCE_C "generatesoucecode"
    ${BUILTIN_EXTENSION_VALA_FILES}
PACKAGES
    ${PKGS}
OPTIONS
    ${VALAFLAGS}
CUSTOM_VAPIS
    ${EXTRA_VAPIS}
    "${MIDORI_SOURCE_DIR}/midori/${MIDORI_PROJECT_NAME}.vapi"
    "${MIDORI_SOURCE_DIR}/katze/katze.vapi"
    "${MIDORI_SOURCE_DIR}/midori/${LIBMIDORI}.vapi"
)
add_library(${BUILTIN_EXTENSION_LIBRARY} MODULE ${BUILTIN_EXTENSION_SOURCE_C})
add_dependencies(${BUILTIN_EXTENSION_LIBRARY} ${LIBMIDORI})
target_link_libraries(${BUILTIN_EXTENSION_LIBRARY} ${LIBMIDORI})
set_target_properties(${BUILTIN_EXTENSION_LIBRARY} PROPERTIES COMPILE_FLAGS "${VALA_CFLAGS}")
install(TARGETS ${BUILTIN_EXTENSION_LIBRARY} LIBRARY DESTINATION ${EXTENSIONDIR})
