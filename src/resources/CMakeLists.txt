install(FILES deepin-app-store.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)

install(FILES themes/common/images/deepin-app-store.svg
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)

install(FILES settings.ini.default
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-app-store/)

install(FILES appstore-keyring.gpg
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/keyrings/)

install(FILES appstore.gpg
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-app-store/)

install(FILES appstore.list
        DESTINATION /etc/apt/sources.list.d/)
