find_package(gflags REQUIRED)

add_executable(carbiok main.cpp)
target_sources(carbiok
PRIVATE
  main.cpp
)
target_include_directories(carbiok PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(carbiok carbio gflags)
