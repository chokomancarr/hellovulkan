set(DIR src)

set(SOURCES
	${SOURCES}
	${DIR}/main.cpp
	${DIR}/vulkanapi.cpp
	PARENT_SCOPE
)
set(HEADERS
	${HEADERS}
	${DIR}/vulkanapi.hpp
	PARENT_SCOPE
)

