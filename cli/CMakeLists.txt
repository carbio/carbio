find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup()

qt_add_resources(QRC_QML ${CMAKE_CURRENT_LIST_DIR}/../res/res.qrc)
qt_add_resources(QRC_RES ${CMAKE_CURRENT_LIST_DIR}/../qml/qml.qrc)

qt_add_executable(carbiod
  main.cpp
  radialbar.cpp
  radialbar.h
  controller.h
  controller.cpp
  admin_authenticator.h
  admin_authenticator.cpp
  session_manager.h
  session_manager.cpp
  audit_logger.h
  audit_logger.cpp
  profile_manager.h
  profile_manager.cpp
  security_types.h
  ${QRC_QML}
  ${QRC_RES}
)

target_include_directories(carbiod PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(OpenSSL REQUIRED)

target_link_libraries(carbiod PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
  carbio
  OpenSSL::Crypto
)

set_target_properties(carbiod PROPERTIES AUTOMOC TRUE)
