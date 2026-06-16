cmake_minimum_required(VERSION 4.3)

# Workaround for CMake 4.x + Qt6 Qml macro compatibility
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Set minimum policy version for Qt6Qml macro compatibility
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# Explicitly set QT_DEFAULT_MAJOR_VERSION before finding Qt6/KF6
set(QT_DEFAULT_MAJOR_VERSION 6)

project(soundkonverter)

set(CMAKE_CXX_STANDARD 11)

# show more warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
# strict ISO C and ISO C++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
# use c++11 standard
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# implicit conversions
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")
# implicit conversions from signed to unsigned
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-conversion")

# hide deprecated declarations warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# kde libs
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(CMakePackageConfigHelpers)
include(ECMInstallIcons)
include(FeatureSummary)

# Find Qt6 explicitly first with Qml component to establish Qt6 context
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Xml Qml)
# Now find KF6 - Qt6Qml will already be available in context
find_package(KF6 REQUIRED COMPONENTS I18n XmlGui KIO Solid Config ConfigWidgets WidgetsAddons)
find_package(KF6Cddb QUIET)
if(NOT KF6Cddb_FOUND)
    find_package(KCddb6 QUIET)
endif()
set(HAVE_CDBD ${KF6Cddb_FOUND} ${KCddb6_FOUND})
# Phonon is optional - try Qt6 version first, fallback to none if not available
find_package(Phonon4Qt6 QUIET)
set(HAVE_PHONON ${Phonon4Qt6_FOUND})


# additional libs
find_package(Cdparanoia REQUIRED)

set(TAGLIB_MIN_VERSION "1.10")
find_package(Taglib ${TAGLIB_MIN_VERSION} REQUIRED)


# plugins 
add_subdirectory(plugins)


# include directories
include_directories(${CDPARANOIA_INCLUDE_DIR})


# translation
include(ECMOptionalAddSubdirectory)
ecm_optional_add_subdirectory( po )


# soundkonvertercore
set(soundkonvertercore_SRCS
   core/backendplugin.cpp
   core/conversionoptions.cpp
   core/codecwidget.cpp
   core/codecplugin.cpp
   core/filterplugin.cpp
   core/replaygainplugin.cpp
   core/ripperplugin.cpp
)

add_library(soundkonvertercore SHARED ${soundkonvertercore_SRCS})
target_link_libraries(soundkonvertercore KF6::I18n KF6::WidgetsAddons KF6::KIOFileWidgets KF6::KIOCore)

install(TARGETS soundkonvertercore DESTINATION ${LIB_INSTALL_DIR})


# soundkonverter
set(soundkonverter_SRCS
   configdialog/configdialog.cpp
   configdialog/configpagebase.cpp
   configdialog/configgeneralpage.cpp
   configdialog/configbackendspage.cpp
   configdialog/configadvancedpage.cpp
   configdialog/configcoverartpage.cpp
   metadata/tagengine.cpp
   metadata/MetaReplayGain.cpp
   opener/fileopener.cpp
   opener/diropener.cpp
   opener/cdopener.cpp
   opener/urlopener.cpp
   opener/playlistopener.cpp
   replaygainscanner/replaygainscanner.cpp
   replaygainscanner/replaygainfilelist.cpp
   replaygainscanner/replaygainfilelistitem.cpp
   replaygainscanner/replaygainprocessor.cpp
   global.cpp
   main.cpp
   soundkonverter.cpp
   soundkonverterapp.cpp
   soundkonverterview.cpp
   codecproblems.cpp
   codecoptimizations.cpp
   combobutton.cpp
   config.cpp
   conversionoptionsmanager.cpp
   convert.cpp
   convertitem.cpp
   filelist.cpp
   filelistitem.cpp
   logger.cpp
   logviewer.cpp
   options.cpp
   optionssimple.cpp
   optionsdetailed.cpp
   optionseditor.cpp
   optionslayer.cpp
   pluginloader.cpp
   progressindicator.cpp
   outputdirectory.cpp
   aboutplugins.cpp
)

add_executable(soundkonverter ${soundkonverter_SRCS})
if(HAVE_PHONON)
    if(KF6Cddb_FOUND)
        target_link_libraries(soundkonverter KF6::XmlGui KF6::WidgetsAddons KF6::KIOFileWidgets KF6::KIOCore KF6::Solid KF6::Cddb Phonon::phonon4qt6 Taglib::Taglib ${CDPARANOIA_LIBRARIES} soundkonvertercore)
    elseif(KCddb6_FOUND)
        target_link_libraries(soundkonverter KF6::XmlGui KF6::WidgetsAddons KF6::KIOFileWidgets KF6::KIOCore KF6::Solid KCddb6 Phonon::phonon4qt6 Taglib::Taglib ${CDPARANOIA_LIBRARIES} soundkonvertercore)
    endif()
else()
    if(KF6Cddb_FOUND)
        target_link_libraries(soundkonverter KF6::XmlGui KF6::WidgetsAddons KF6::KIOFileWidgets KF6::KIOCore KF6::Solid KF6::Cddb Taglib::Taglib ${CDPARANOIA_LIBRARIES} soundkonvertercore)
    elseif(KCddb6_FOUND)
        target_link_libraries(soundkonverter KF6::XmlGui KF6::WidgetsAddons KF6::KIOFileWidgets KF6::KIOCore KF6::Solid KCddb6 Taglib::Taglib ${CDPARANOIA_LIBRARIES} soundkonvertercore)
    endif()
endif()

install(TARGETS soundkonverter DESTINATION ${BIN_INSTALL_DIR})


# install directories
install(FILES soundkonverter.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES soundkonverterui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/soundkonverter)

install(FILES soundkonverter_codecplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES soundkonverter_filterplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES soundkonverter_replaygainplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES soundkonverter_ripperplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES soundkonverter-rip-audiocd.desktop DESTINATION ${DATA_INSTALL_DIR}/solid/actions)

install(FILES soundkonverter.appdata.xml DESTINATION ${SHARE_INSTALL_PREFIX}/metainfo)

add_subdirectory(icons)
add_subdirectory(images)


# summary
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
