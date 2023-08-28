add_executable(fbae
  main.cpp
  options.cpp options.h OptParserExtended.cpp OptParserExtended.h
  Measures.cpp Measures.h
        Param.cpp Param.h)
target_link_libraries(fbae PUBLIC enet)
if (UNIX)
  target_link_options(fbae PUBLIC -pthread)
endif()
