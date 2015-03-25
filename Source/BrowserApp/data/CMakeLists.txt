# Copyright (C) 2013 Christian Dywan <christian@twotoasts.de>
# 资源生成到 bin/res路径下，注意要维护DATA_FILES，将不需要生成到bin下的资源REMOVE_ITEM，否则会导致cmake失败。install路径待实现。

include(FindConvert)
if (NOT CONVERT_FOUND)
    message(FATAL_ERROR "rsvg-convert not found")
endif ()

include(FindIntltool)
if (NOT INTLTOOL_MERGE_FOUND)
    message(FATAL_ERROR "intltool-merge not found")
elseif (NOT INTLTOOL_UPDATE_FOUND)
    message(FATAL_ERROR "intltool-update not found")
endif ()

# ZRL remove CMakeFiles/ firstly
file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles/)

file(GLOB_RECURSE DATA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
list(REMOVE_ITEM DATA_FILES "CMakeLists.txt")
list(REMOVE_ITEM DATA_FILES "cmake_install.cmake" "CTestTestfile.cmake" "Makefile" "cdosbrowser.appdata.xml.in" "cdosbrowser-private.desktop" "cdosbrowser.desktop"
                            "faq.html" "faq.css")

# 去掉隐私模式外部启动标志
list(REMOVE_ITEM DATA_FILES "cdosbrowser-private.desktop.in")

foreach(FILE ${DATA_FILES})
    if (${FILE} MATCHES "faq.")
        install(FILES ${FILE} DESTINATION ${CMAKE_INSTALL_DOCDIR})
    elseif (${FILE} MATCHES ".desktop")
        if (NOT WIN32)
            string(REPLACE ".desktop.in" "" DESKTOP_ID ${FILE})
            INTLTOOL_MERGE_DESKTOP (${DESKTOP_ID} po)
        endif ()
    elseif (${FILE} MATCHES ".appdata.xml")
        if (NOT WIN32)
            string(REPLACE ".appdata.xml.in" "" DESKTOP_ID ${FILE})
            INTLTOOL_MERGE_APPDATA (${DESKTOP_ID} po)
        endif ()
    elseif (${FILE} MATCHES "\\.svg$")
        string(REPLACE ".svg" "" IMG_ID ${FILE})
        string (FIND ${FILE} "/" IS_DIR)
        if (IS_DIR GREATER -1)
            string(REPLACE "/" ";" DIR_LIST ${FILE})
            LIST(GET DIR_LIST 0 S_DIR)
            SVG2PNG (${IMG_ID} "${CMAKE_INSTALL_DATADIR}/${MIDORI_PROJECT_NAME}/res/${S_DIR}")
        else ()
            SVG2PNG (${IMG_ID} "${CMAKE_INSTALL_DATADIR}/${MIDORI_PROJECT_NAME}/res/")
        endif()
    # These are being handled in add_executable for the "midori" binary
    elseif (${FILE} MATCHES "\\.ico$")
    elseif (${FILE} MATCHES "\\.rc$")
    # This is only meant for testing, and not used in production
    elseif (${FILE} MATCHES "\\.swf$")
    else()
        string(FIND ${FILE} "/" IS_DIR)
        if (IS_DIR GREATER -1)
            string(REPLACE "/" ";" DIR_LIST ${FILE})
            LIST(GET DIR_LIST 0 S_DIR)
            LIST(GET DIR_LIST 1 S_FILE)
            install(FILES ${S_DIR}/${S_FILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${MIDORI_PROJECT_NAME}/res/${S_DIR})
        else ()
            install(FILES ${FILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${MIDORI_PROJECT_NAME}/res/)
        endif()
    endif()

endforeach()
