if(TARGET libapng_static)
    return()
endif()

message(STATUS "Third-party: creating target 'libapng_static'")

set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_BUILD_ZLIB ON CACHE BOOL "" FORCE)

include(FetchContent)

	FetchContent_Declare(
		libapng-local
		SOURCE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libapng/"
		EXCLUDE_FROM_ALL ON
		DOWNLOAD_ONLY YES
		OPTION
			"PNG_SHARED OFF"
			"PNG_EXECUTABLES OFF"
			"PNG_TESTS OFF"
			"PNG_BUILD_ZLIB ON"
	)

FetchContent_MakeAvailable(libapng-local)











set(libapng_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libapng" "${libapng_BINARY_DIR}" CACHE PATH "" FORCE)

set(libapng_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/libapng/png.h" "${CMAKE_CURRENT_SOURCE_DIR}/libapng/pngconf.h" CACHE PATH "" FORCE)
