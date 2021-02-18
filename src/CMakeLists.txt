
set(HEADERS
  	common.h
	config.h
	distfs.h
	utils.h
	log.h
	config_json.h
	use_intrins.h
	SOM.h
	AsyncSom.h
	DatasetFeatures.h
	DatasetFrames.h
	KeywordRanker.h
	RelevanceScores.h
  	SomHunter.h
	Submitter.h
	Filters.h
	UserContext.h
	SearchContext.h
	ImageManipulator.h
	CollageRanker.h
)

set(SOURCES
	${HEADERS}
	main.cpp
	SOM.cpp
	AsyncSom.cpp
	DatasetFeatures.cpp
	DatasetFrames.cpp
	KeywordRanker.cpp
	RelevanceScores.cpp
  	SomHunter.cpp
	Submitter.cpp
	Filters.cpp
	UserContext.cpp
	SearchContext.cpp
	ImageManipulator.cpp
	CollageRanker.cpp
)

# 
# Create the binary
#
add_executable(somhunter
	${SOURCES}
	${HEADERS}
)

# 
# Link it with the libraries
#
target_include_directories(somhunter 
	PRIVATE
		${CURL_INCLUDE_DIR}
)

target_link_libraries(somhunter 
	PRIVATE
		${CURL_LIBRARIES}
		${TORCH_LIBRARIES}
		Threads::Threads
		#opencv_ml opencv_dnn opencv_core opencv_flann # Perarations
)


# 
# Setup its properties
#
set_target_properties(somhunter PROPERTIES CXX_STANDARD 17)

if (MSVC) 
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll") 
  add_custom_command(TARGET somhunter 
                     POST_BUILD 
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                     ${TORCH_DLLS} 
                     $<TARGET_FILE_DIR:somhunter>) 
endif (MSVC)