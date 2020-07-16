cmake_minimum_required( VERSION 2.6 )

project( ITK_graphicslib )

if( MSVC )
  # find SDL?
  include_directories( $ENV{HOME}/include )
  link_directories( $ENV{HOME}/lib )

  add_compile_options( -W4 )
  if( CMAKE_BUILD_TYPE MATCHES Debug )
    add_compile_options( -DEBUG )
  elseif( CMAKE_BUILD_TYPE MATCHES Release )
    add_compile_options( -O2 )
  endif()

  link_libraries (
    SDL2.dll
    SDL2_ttf.dll
  )
else()
  find_package( SDL REQUIRED )
  find_package( SDL_ttf REQUIRED )

  if( NOT SDLTTF_FOUND )
    message ( FATAL_ERROR "SDL_ttf not found!" )
  endif ()

  add_compile_options( -Wall -Wextra)
  if( CMAKE_BUILD_TYPE MATCHES Debug )
    add_compile_options( -g )
  elseif( CMAKE_BUILD_TYPE MATCHES Release )
    add_compile_options( -O3 )
  endif()

  link_libraries (
    SDL2
    SDL2_ttf
    SDLmain
  )
endif()

add_library( graphics2 graphics2.cpp )
add_executable( test_graphics main.cpp )

target_link_libraries( test_graphics graphics2 )
