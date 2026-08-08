

add_library(pngwriter 
  pngwriter.cpp
  pngconf.h
  pngwriter.h
)

target_include_directories (pngwriter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pngwriter PNG::PNG)

