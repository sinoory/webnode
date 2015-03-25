# Copyright (C) 2013 Olivier Duchateau
# ZRL 将config下文件安装到/usr/local/cdosbrowser/路径下,目前从config中只安装adblock的配置文件

set (SYSCONFDIR ${CMAKE_INSTALL_FULL_SYSCONFDIR})
set (XDG_CONFIG_DIR "${MIDORI_PROJECT_NAME}")

file (GLOB_RECURSE CONFIG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
list (REMOVE_ITEM CONFIG_FILES "CMakeLists.txt" "cmake_install.cmake" "CTestTestfile.cmake" "Makefile" "CMakeDirectoryInformation.cmake" "progress.marks")

if (${CMAKE_INSTALL_PREFIX} STREQUAL "/usr")
    set(CMAKE_INSTALL_SYSCONFDIR ${CMAKE_INSTALL_FULL_DATADIR})
endif()

foreach (FILE ${CONFIG_FILES})
    string (FIND ${FILE} "adblock" ADBLOCK_CONF)
    if (ADBLOCK_CONF GREATER -1)
        string (REPLACE "config" "" dirname ${FILE})
        install (FILES ${FILE}
            DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/${XDG_CONFIG_DIR}/${dirname}")
#    else ()
#        install (FILES ${FILE}
#            DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/${XDG_CONFIG_DIR}")
    endif ()
endforeach ()
