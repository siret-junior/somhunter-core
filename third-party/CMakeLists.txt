
target_include_directories(${SOMHUNTER_TARGET} SYSTEM
	PUBLIC
		stb
		json11
		json
		cereal/include
		hash-library
)

target_include_directories(${SOMHUNTER_TARGET_TESTS} SYSTEM
	PUBLIC
		stb
		json11
		json
		cereal/include
		hash-library
)

set(HEADERS
	json11/json11.hpp
	json/nlohmann/json.hpp
	hash-library/sha256.h
)

set(SOURCES
	${HEADERS}
	json11/json11.cpp
	hash-library/sha256.cpp
)

target_sources(${SOMHUNTER_TARGET}  
	PUBLIC
		${SOURCES}
)

target_sources(${SOMHUNTER_TARGET_TESTS}  
	PUBLIC
		${SOURCES}
)