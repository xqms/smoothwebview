# Project Needs a name ofcourse
project(plasma-smoothwebview)

# Find the required Libaries
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )

# We add our source code here
set(smoothwebview_SRCS smoothwebview.cpp)

kde4_add_ui_files(smoothwebview_SRCS smoothwebviewconfig.ui)

# Now make sure all files get to the right place
kde4_add_plugin(plasma_applet_smoothwebview ${smoothwebview_SRCS})
target_link_libraries(plasma_applet_smoothwebview 
                      ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KDEWEBKIT_LIBS} ${QT_QTWEBKIT_LIBRARY})

install(TARGETS plasma_applet_smoothwebview
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-smoothwebview.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
