# Kasturisundari Chain — Apps CMake Configuration

add_executable(kasturid kasturid/main.cpp)
target_link_libraries(kasturid PRIVATE kasturisundari_core)

add_executable(kasturi-cli kasturi-cli/main.cpp)
target_link_libraries(kasturi-cli PRIVATE kasturisundari_core)
