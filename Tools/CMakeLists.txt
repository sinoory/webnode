# ZRL disable some test runner. Should enable them.

#if (ENABLE_WEBKIT2)
#    add_subdirectory(WebKitTestRunner)
#endif ()

if ("${PORT}" STREQUAL "Efl")
    add_subdirectory(ImageDiff)

    add_subdirectory(MiniBrowser/efl)
    add_subdirectory(DumpRenderTree/TestNetscapePlugIn)
elseif ("${PORT}" STREQUAL "GTK")
#    add_subdirectory(ImageDiff)
    if (ENABLE_WEBKIT2)
#        add_subdirectory(MiniBrowser/gtk)
#        add_subdirectory(TestWebKitAPI/Tests/WebKit2Gtk)
    endif ()
    if (ENABLE_X11_TARGET)
#        add_subdirectory(DumpRenderTree/TestNetscapePlugIn)
    endif ()
elseif ("${PORT}" STREQUAL "WinCE")
    add_subdirectory(WinCELauncher)
endif ()

if (ENABLE_WEBKIT2 AND ENABLE_API_TESTS)
#    add_subdirectory(TestWebKitAPI)
endif ()
