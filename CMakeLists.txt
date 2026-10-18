cmake_minimum_required(VERSION 3.20)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
project(wallet_image VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# raylib (pinned). Set FETCHCONTENT_SOURCE_DIR_RAYLIB to use a local checkout.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

add_executable(wallet_image MACOSX_BUNDLE
  src/main.cpp
  src/window.cpp
  src/content.cpp
  src/anim.cpp
  src/render.cpp
  src/wallet.cpp
  src/photo.cpp
  src/note.cpp
  src/gothic.cpp)

target_link_libraries(wallet_image PRIVATE raylib)
# Direct GLFW access (cursor polling while mouse-passthrough is on).
target_include_directories(wallet_image PRIVATE ${raylib_SOURCE_DIR}/src/external/glfw/include)

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(MSVC)
    target_link_options(wallet_image PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
  else()
    target_link_options(wallet_image PRIVATE -mwindows)
  endif()
endif()

if(APPLE)
  # Builds "Wallet Keepsake.app"; assets go in Contents/Resources.
  set_target_properties(wallet_image PROPERTIES
    OUTPUT_NAME "Wallet Keepsake"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.walletkeepsake.app"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
  set(ASSET_DEST $<TARGET_BUNDLE_CONTENT_DIR:wallet_image>/Resources/assets)
else()
  set(ASSET_DEST $<TARGET_FILE_DIR:wallet_image>/assets)
endif()

add_custom_command(TARGET wallet_image POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${ASSET_DEST})
