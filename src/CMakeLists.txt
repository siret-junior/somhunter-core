
set(HEADERS
  	common.h
	config.h
	distfs.h
	utils.h
	utils_json.h
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
	query_types.h
	UserContext.h
	SearchContext.h
	ImageManipulator.h
	CanvasQueryRanker.h
	EmbeddingRanker.h
	RelocationRanker.h

	#Extractor.h
	NetworkApi.h
)

set(SOURCES
	${HEADERS}
	SOM.cpp
	AsyncSom.cpp
	DatasetFeatures.cpp
	DatasetFrames.cpp
	KeywordRanker.cpp
	RelevanceScores.cpp
  	SomHunter.cpp
	Submitter.cpp
	query_types.cpp
	UserContext.cpp
	SearchContext.cpp
	ImageManipulator.cpp
	CanvasQueryRanker.cpp
	EmbeddingRanker.cpp
	RelocationRanker.cpp

	#Extractor.cpp
	NetworkApi.cpp
)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS} main.cpp)
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})
