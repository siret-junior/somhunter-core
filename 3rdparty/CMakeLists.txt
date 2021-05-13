
target_include_directories(${SOMHUNTER_TARGET}  
	PUBLIC
		stb
		json11
		cereal/include
		hash-library
)

target_include_directories(${SOMHUNTER_TARGET_TESTS}  
	PUBLIC
		stb
		json11
		cereal/include
		hash-library
)

set(HEADERS
	json11/json11.hpp
	json11/json11.hpp
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