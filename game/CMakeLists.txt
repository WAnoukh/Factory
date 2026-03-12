include_directories(${INLUDES_PATH})

add_library(game STATIC)

target_sources(game 
    PRIVATE 
    ./src/game.c
    ./src/game_runtime.c
    ./src/game_commands.c

    ./src/camera/top_down_camera.c

    ./src/level/level.c

    ./src/gameplay/workpool.c
    ./src/gameplay/worker.c
    ./src/gameplay/worker_behaviour.c

    ./src/behaviour_tree/behaviour_tree.c
)

target_link_libraries(game
    core
    engine
)

target_include_directories(game
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${INLUDES_PATH}/
)

set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
