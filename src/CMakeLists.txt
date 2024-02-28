cmake_minimum_required(VERSION 3.16)

project(Mebo_MK3_Converter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_PREFIX_PATH "D:/Qt/6.6.2/msvc2022_64_static")
find_package(Qt6 6.4 REQUIRED COMPONENTS Quick)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

qt_add_executable(appMebo_MK3_Converter
    resources.qrc
    main.cpp
)

qt_add_qml_module(appMebo_MK3_Converter
    URI Mebo_MK3_Converter
    VERSION 1.0
    QML_FILES Main.qml
    SOURCES Converter.h
    RESOURCES Logo_orange.svg
    RESOURCES resources.qrc
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appMebo_MK3_Converter PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appMebo_MK3_Converter
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMebo_MK3_Converter
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appMebo_MK3_Converter
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
