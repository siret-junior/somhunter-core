
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
		Threads::Threads
)

# 
# Setup its properties
#
set_target_properties(somhunter PROPERTIES CXX_STANDARD 17)