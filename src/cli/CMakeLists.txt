add_executable(carbio_cli main.cpp)
target_sources(carbio_cli
PRIVATE
  main.cpp
  clear_menu.cpp
  delete_menu.cpp
  device_config_menu.cpp
  enroll_menu.cpp
  identify_menu.cpp
  led_config_menu.cpp
  main_menu.cpp
  verify_menu.cpp
)
target_include_directories(carbio_cli PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(carbio_cli carbio)
