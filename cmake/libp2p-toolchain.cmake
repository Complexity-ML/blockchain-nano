# Shared by cpp-libp2p and Hunter's nested builds.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    # fmt bundled by this libp2p release is incompatible with AppleClang 21 consteval.
    set(CMAKE_CXX_FLAGS_INIT "-DFMT_CONSTEVAL=")
endif()
