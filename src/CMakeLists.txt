
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Xml REQUIRED)
# add dtk log
# find_package(DtkCore REQUIRED)

find_package(PkgConfig REQUIRED)
# libdbus-1-dev 添加dbus api支持
pkg_search_module(DBUS REQUIRED dbus-1)

aux_source_directory(proxy PROXY_SRC)
aux_source_directory(message MSG_SRC)
aux_source_directory(filter FILTER_SRC)

#include_directories(proxy PROXY_INCLUDE)
#include_directories(message MSG_INCLUDE)
#include_directories(filter FILTER_INCLUDE)

# libpolkit-qt5-1-dev libpolkit-agent-1-dev
find_package(PolkitQt5-1)

include_directories(${DBUS_INCLUDE_DIRS})

set(MAIN_SOURCES
        main.cpp
        ${PROXY_SRC}
        ${FILTER_SRC}
        ${MSG_SRC}
        )

set(LINK_LIBS
    stdc++
    Qt5::Core
    Qt5::Network
    Qt5::DBus
    Qt5::Xml
    dtkcore
    ${DBUS_LIBRARIES}
    PolkitQt5-1::Agent
    )

add_executable(polkit-test
        ${MAIN_SOURCES})

target_link_libraries(polkit-test
                      PRIVATE ${LINK_LIBS})
#target_include_directories(ll-dbusProxy PRIVATE ${PROXY_INCLUDE} ${MSG_INCLUDE} ${FILTER_INCLUDE})

#设置生成目标二进制的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
install(TARGETS polkit-test RUNTIME DESTINATION bin)