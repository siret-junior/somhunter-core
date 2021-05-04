
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

	Extractor.h
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

	Extractor.cpp
)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
