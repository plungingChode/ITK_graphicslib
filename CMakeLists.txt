cmake_minimum_required( VERSION 2.6 )

project( ITK_graphicslib )

if (MSVC)
  # find SDL?
  include_directories( $ENV{HOME}/include )
  link_directories( $ENV{HOME}/lib )

  add_compile_options(/W4)

  link_libraries (
    SDL2.dll
    SDL2_ttf.dll
  )
else ()
  link_libraries (
    SDL
    SDL_ttf
    SDLmain
  )
  find_package( SDL REQUIRED )
  find_package( SDL_ttf REQUIRED )

  if (NOT SDLTTF_FOUND)
    message ( FATAL_ERROR "SDL_ttf not found!" )
  endif ()
endif ()

add_library( graphics graphics.cpp )
add_executable( test_graphics main.cpp )

target_link_libraries( test_graphics graphics )
