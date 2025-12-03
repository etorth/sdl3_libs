find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(sdl3_test main.cpp)
target_link_libraries(sdl3_test PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
