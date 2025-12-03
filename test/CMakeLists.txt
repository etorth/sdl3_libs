find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(SDL3TextExample main.cpp)

# Link libraries
target_link_libraries(SDL3TextExample PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
