option(USE_SYSTEM_RESOURCES "Use web resources in source tree or in system" OFF)

if(USE_SYSTEM_RESOURCES)
  add_definitions(
    -DDSTORE_WEB_DIR="${CMAKE_INSTALL_PREFIX}/share/deepin-app-store/web_dist")
else()
  add_definitions(-DDSTORE_WEB_DIR="${CMAKE_CURRENT_SOURCE_DIR}/web_dist")
endif()

if(DISABLE_MULTI_THREAD)
  add_definitions(-DDSTORE_DISABLE_MULTI_THREAD="YES")
endif()

if(DISABLE_DXCB)
  add_definitions(-DDSTORE_NO_DXCB)
endif()

if(CONFIG_VERSION)
  add_definitions(-DVERSION=${CONFIG_VERSION})
endif()

add_subdirectory(dbus)
add_subdirectory(resources)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_definitions("-DQT_MESSAGELOGCONTEXT")

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5LinguistTools)
find_package(Qt5Sql REQUIRED)
find_package(Qt5WebChannel REQUIRED)
find_package(Qt5WebEngineWidgets REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5WebSockets REQUIRED)

pkg_search_module(DtkWidget REQUIRED dtkwidget)
pkg_search_module(DtkFrameWorkDBus REQUIRED dframeworkdbus)

include_directories(${DtkFrameWorkDBus_INCLUDE_DIRS})
include_directories(${DtkWidget_INCLUDE_DIRS})
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

set(Qt_LIBS Qt5::Core Qt5::DBus Qt5::Svg Qt5::Sql Qt5::Widgets Qt5::WebChannel Qt5::WebSockets Qt5::WebEngineWidgets)

set(LINK_LIBS
    ${Qt_LIBS}
    ${DtkWidget_LDFLAGS}
    ${DtkFrameWorkDBus_LDFLAGS})

set(BASE_FILES
    base/sysinfo.cpp
    base/sysinfo.h
    base/command.cpp
    base/command.h
    base/consts.cpp
    base/consts.h
    base/file_util.cpp
    base/file_util.h
    base/launcher.cpp
    base/launcher.h)

set(DBUS_FILES
    dbus/dbus_variant/app_metadata.cpp
    dbus/dbus_variant/app_metadata.h
    dbus/dbus_variant/app_version.cpp
    dbus/dbus_variant/app_version.h
    dbus/dbus_variant/app_version.h
    dbus/dbus_variant/installed_app_info.cpp
    dbus/dbus_variant/installed_app_info.h
    dbus/dbus_variant/installed_app_timestamp.cpp
    dbus/dbus_variant/installed_app_timestamp.h
    dbus/app_store_dbus_adapter.cpp
    dbus/app_store_dbus_adapter.h
    dbus/app_store_dbus_interface.cpp
    dbus/app_store_dbus_interface.h
    dbus/dbus_consts.cpp
    dbus/dbus_consts.h
    dbus/deepinid_interface.h
    dbus/deepinid_interface.cpp
    dbus/dbus_extended_abstract_interface.cpp
    dbus/dbus_extended_abstract_interface.h
    dbus/lastore_deb_interface.cpp
    dbus/lastore_deb_interface.h
    dbus/sys_lastore_job_interface.cpp
    dbus/sys_lastore_job_interface.h
    dbus/lastore_job_interface.cpp
    dbus/lastore_job_interface.h)

set(RESOURCES_FILES
    resources/images.cpp
    resources/images.h
    resources/theme.cpp
    resources/theme.h
    resources/themes/themes.qrc)

set(SERVICES_FILES
    services/rcc_scheme_handler.cpp
    services/rcc_scheme_handler.h
    services/dbus_manager.cpp
    services/dbus_manager.h
    services/rcc_scheme_handler.cpp
    services/rcc_scheme_handler.h
    services/search_result.cpp
    services/search_result.h
    services/session_manager.cpp
    services/session_manager.h
    services/settings_manager.cpp
    services/settings_manager.h
    services/account_manager.h
    services/account_manager.cpp
    services/store_daemon_manager.cpp
    services/store_daemon_manager.h
    services/package/package_manager_interface.h
    services/package/package_manager_interface.cpp
    services/package/package_manager.h
    services/package/package_manager.cpp
    services/package/apt_package_manager.h
    services/package/apt_package_manager.cpp
    services/package/apt_util_worker.cpp
    services/package/apt_util_worker.h
    services/package/dpk_url.h
    services/package/dpk_url.cpp)

set(UI_FILES
    ui/web_window.cpp
    ui/web_window.h
    ui/channel/image_viewer_proxy.cpp
    ui/channel/image_viewer_proxy.h
    ui/channel/log_proxy.cpp
    ui/channel/log_proxy.h
    ui/channel/menu_proxy.cpp
    ui/channel/menu_proxy.h
    ui/channel/search_proxy.cpp
    ui/channel/search_proxy.h
    ui/channel/settings_proxy.cpp
    ui/channel/settings_proxy.h
    ui/channel/store_daemon_proxy.cpp
    ui/channel/store_daemon_proxy.h
    ui/channel/channel_proxy.h
    ui/channel/account_proxy.h
    ui/channel/account_proxy.cpp
    ui/channel/websocket_proxy.cpp
    ui/channel/websocket_proxy.h
    ui/widgets/image_viewer.cpp
    ui/widgets/image_viewer.h
    ui/widgets/search_button.cpp
    ui/widgets/search_button.h
    ui/widgets/search_completion_window.cpp
    ui/widgets/search_completion_window.h
    ui/widgets/search_edit.cpp
    ui/widgets/search_edit.h
    ui/widgets/title_bar.cpp
    ui/widgets/title_bar.h
    ui/widgets/user_menu.h
    ui/widgets/user_menu.cpp
    ui/widgets/title_bar_menu.cpp
    ui/widgets/title_bar_menu.h)

set(PACKAGE_FILES
    daemon/pkgmanager/lastorejobmanager.cpp
    daemon/pkgmanager/lastorejobmanager.h
    daemon/pkgmanager/lastorejobservice.cpp
    daemon/pkgmanager/lastorejobservice.h
    daemon/pkgmanager/metadatamanager.cpp
    daemon/pkgmanager/metadatamanager.h
    daemon/pkgmanager/pkgmanagerservice.cpp
    daemon/pkgmanager/pkgmanagerservice.h)

set(SETTINGS_FILES
    daemon/settings/settingservice.cpp
    daemon/settings/settingservice.h )

# Executable files Generate .qm files from .ts files.
file(GLOB TRANSLATION_TS
     ${CMAKE_SOURCE_DIR}/translations/deepin-app-store*.ts)
set_source_files_properties(${TRANSLATION_TS}
                            PROPERTIES
                            OUTPUT_LOCATION
                            ${CMAKE_BINARY_DIR}/translations)
qt5_add_translation(TRANSLATION_QM ${TRANSLATION_TS})
add_custom_target(update-qm DEPENDS ${TRANSLATION_QM})

add_custom_command(
  OUTPUT deepin-update-desktop
  COMMAND
    sh -c
    "deepin-desktop-ts-convert ts2desktop resources/deepin-app-store.desktop ../translations/desktop resources/deepin-app-store.desktop"
  )

add_executable(deepin-app-store-daemon
               daemon/main.cpp
               ${PACKAGE_FILES}
               ${DBUS_FILES}
               ${SETTINGS_FILES})
target_link_libraries(deepin-app-store-daemon ${LINK_LIBS})

add_custom_target(app-store-daemon DEPENDS deepin-app-store-daemon)
add_custom_target(update-desktop DEPENDS deepin-update-desktop)

add_executable(deepin-app-store
               app/deepin_app_store.cpp
               ${BASE_FILES}
               ${DBUS_FILES}
               ${RESOURCES_FILES}
               ${SERVICES_FILES}
               ${UI_FILES})

target_link_libraries(deepin-app-store ${LINK_LIBS})

if(CMAKE_BUILD_TYPE MATCHES Release)
  add_dependencies(deepin-app-store
                   update-qm
                   update-desktop
                   app-store-daemon)
else()
  add_dependencies(deepin-app-store app-store-daemon)
endif()

add_executable(deepin-app-store-tool app/tool.cpp)
target_link_libraries(deepin-app-store-tool ${LINK_LIBS})

add_executable(deepin-app-store-pkgcache app/pkg_cache.cpp)
target_link_libraries(deepin-app-store-pkgcache ${LINK_LIBS})

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_executable(test-launcher app/test_launcher.cpp base/launcher.cpp
                               base/launcher.h)
  target_link_libraries(test-launcher ${LINK_LIBS})
  add_executable(test-base64-image app/base64_image.cpp base/file_util.cpp
                                   base/file_util.h)
  target_link_libraries(test-base64-image ${LINK_LIBS})

  add_executable(qurl-demo app/qurl_demo.cpp)
  target_link_libraries(qurl-demo ${LINK_LIBS})

  add_executable(test-image-viewer
                 app/test_image_viewer.cpp
                 base/file_util.cpp
                 base/file_util.h
                 ui/widgets/image_viewer.cpp
                 ui/widgets/image_viewer.h
                 resources/themes/themes.qrc)
  target_link_libraries(test-image-viewer ${LINK_LIBS})
endif()

install(TARGETS deepin-app-store DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(TARGETS deepin-app-store-tool DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(TARGETS deepin-app-store-pkgcache DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(TARGETS deepin-app-store-daemon DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/daemon/data/com.deepin.AppStore.json
        DESTINATION /etc/deepin/pusher/handler.d)

install(DIRECTORY ${CMAKE_BINARY_DIR}/translations
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-app-store/
        FILES_MATCHING
        PATTERN "*")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/daemon/config/pkg-config-hook-config
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-app-store/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/daemon/config/99appstore.conf
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-app-store/)

