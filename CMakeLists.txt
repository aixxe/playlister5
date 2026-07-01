cmake_minimum_required(VERSION 3.25)

project(playlister VERSION 5.0.2 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GenerateImportLib)
include(VersionInformation)

find_package(fmt CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(safetyhook CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/bm2dx.cc
  src/memory.cc
  src/modules.cc
  src/playlists.cc
  src/playlister.cc
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_link_import_library(${PROJECT_NAME} avs2-core
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/avs2-core.def"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  fmt::fmt
  glaze::glaze
  safetyhook::safetyhook
)

if (MINGW)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
  "META_PROJECT_VERSION=\"${PROJECT_VERSION}\""
  "META_GIT_BRANCH=\"${META_GIT_BRANCH}\""
  "META_GIT_COMMIT_SHORT=\"${META_GIT_COMMIT_SHORT}\""
  "META_COMPILER_ID=\"${CMAKE_CXX_COMPILER_ID}\""
  "META_COMPILER_VERSION=\"${CMAKE_CXX_COMPILER_VERSION}\""
)