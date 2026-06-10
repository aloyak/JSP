include(FetchContent)

FetchContent_Declare(
    sgp4
    GIT_REPOSITORY https://github.com/dnwrnr/sgp4.git
    GIT_TAG master
)
FetchContent_MakeAvailable(sgp4)