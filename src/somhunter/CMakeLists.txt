
set(HEADERS
	query-types.h
	SomHunter.h
	SOM.h
	AsyncSom.h
	DatasetFeatures.h
	DatasetFrames.h
	KeywordRanker.h
	RelevanceScores.h
	Submitter.h
	UserContext.h
	SearchContext.h
	ImageManipulator.h
	CanvasQueryRanker.h
	EmbeddingRanker.h
	RelocationRanker.h
)

set(SOURCES
	${HEADERS}
	query-types.cpp
	SomHunter.cpp
	SOM.cpp
	AsyncSom.cpp
	DatasetFeatures.cpp
	DatasetFrames.cpp
	KeywordRanker.cpp
	RelevanceScores.cpp
	Submitter.cpp
	UserContext.cpp
	SearchContext.cpp
	ImageManipulator.cpp
	CanvasQueryRanker.cpp
	EmbeddingRanker.cpp
	RelocationRanker.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})

add_subdirectory(api)
add_subdirectory(evaluation-server)
add_subdirectory(logs)
