cmake_minimum_required(VERSION 3.0)
project (hellovulkan)

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
SET(CMAKE_SKIP_BUILD_RPATH FALSE)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(VULKAN_SDK /Users/yasuoka/vulkansdk/macOS)
set(VK_ICD_FILENAMES ${VULKAN_SDK}/etc/vulkan/icd.d/MoltenVK_icd.json)

set(VULKAN /Users/yasuoka/vulkansdk/macOS)

if (MSVC)
	add_definitions(-DPLATFORM_WIN -Dssize_t=SSIZE_T)
elseif (LINUX)
	add_definitions(-DPLATFORM_LNX)
elseif (APPLE)
	add_definitions(-DPLATFORM_MAC)
else()
	message(FATAL_ERROR "Platform not supported!")
endif()

if (MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc /bigobj")
else()
    SET(CMAKE_CXX_FLAGS "-std=c++11 -g -O0 -Wall")
endif()

set(INC_DIRS
	${PROJECT_SOURCE_DIR}/src
	${GLFW3_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${VULKAN_SDK}/include
)

set(LIBS
	glfw
	${GLM_LIBRARY}
)

if (MSVC)
	set(DLLS
	)
elseif (LINUX)
	set(DLLS
	)
else()
	set(LINK_DIRS
		${VULKAN_SDK}/lib/
	)
	set(LIBS
		${LIBS}
		vulkan
		"-framework CoreGraphics"
	)
	set(DLLS
	)
endif()

set(SOURCES)
set(HEADERS)

include_directories(${INC_DIRS})
link_directories(${LINK_DIRS})

add_subdirectory("src")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(hellovulkan ${SOURCES} ${HEADERS})
target_link_libraries(hellovulkan ${LIBS})