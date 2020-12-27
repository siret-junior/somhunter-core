
target_include_directories(somhunter 
	PUBLIC
		stb
		json11
)

set(HEADERS
	stb/stb.h
	stb/stb_image.h
	stb/stb_image_resize.h
	stb/stb_image_write.h

	json11/json11.hpp
)

set(SOURCES
	${HEADERS}
	json11/json11.cpp
)

target_sources(somhunter 
	PUBLIC
		${SOURCES}
)