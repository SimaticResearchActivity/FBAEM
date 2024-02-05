add_executable(fbae
  main.cpp
  options.cpp options.h OptParserExtended.cpp OptParserExtended.h
        Measures.cpp Measures.h
        Param.cpp Param.h
        AlgoLayer/AlgoLayer.cpp
        AlgoLayer/AlgoLayer.h
        SessionLayer/SessionLayer.cpp
        SessionLayer/SessionLayer.h
        AlgoLayer/BBOBBAlgoLayer/BBOBBAlgoLayer.cpp
        AlgoLayer/BBOBBAlgoLayer/BBOBBAlgoLayer.h
        AlgoLayer/BBOBBAlgoLayer/BBOBBAlgoLayerMsg.h
        AlgoLayer/SequencerAlgoLayer/SequencerAlgoLayer.cpp
        AlgoLayer/SequencerAlgoLayer/SequencerAlgoLayer.h
        AlgoLayer/SequencerAlgoLayer/SequencerAlgoLayerMsg.h
        msgTemplates.h
        SessionLayer/SessionLayerMsg.h
        SessionLayer/SessionLayerMsg.h
        basicTypes.h
        get_cpu_time.cpp
        get_cpu_time.h)
target_link_libraries(fbae PUBLIC enet)
target_link_libraries(fbae  PRIVATE MPI::MPI_CXX)
if (WIN32)
  target_link_libraries(fbae PUBLIC ws2_32.lib winmm.lib)
else() # We are under UNIX
  target_link_options(fbae PUBLIC -pthread)
endif()

