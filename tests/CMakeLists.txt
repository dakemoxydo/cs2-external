add_executable(cs2overlay_offset_tests
  offset_parser_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/core/sdk/offset_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/core/sdk/offsets.cpp
)
target_include_directories(cs2overlay_offset_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(cs2overlay_offset_tests PRIVATE nlohmann_json::nlohmann_json)
if(MSVC)
  target_compile_options(cs2overlay_offset_tests PRIVATE /W4 /utf-8 /permissive-)
endif()
add_test(NAME cs2overlay.offset_parser COMMAND cs2overlay_offset_tests)

add_executable(cs2overlay_game_state_tests
  game_state_store_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/core/game/game_state_store.cpp
)
target_include_directories(cs2overlay_game_state_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(cs2overlay_game_state_tests PRIVATE /W4 /utf-8 /permissive-)
endif()
add_test(NAME cs2overlay.game_state_store COMMAND cs2overlay_game_state_tests)
