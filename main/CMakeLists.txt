include(FetchContent)

FetchContent_Declare(
gsl
GIT_REPOSITORY git@github.com:microsoft/GSL.git
GIT_TAG v4.0.0
)
FetchContent_MakeAvailable(gsl)

FetchContent_Declare(
etl
GIT_REPOSITORY git@github.com:ETLCPP/etl.git
GIT_TAG 20.38.17
)
FetchContent_MakeAvailable(etl)

idf_component_register(SRCS "main.cpp"
    INCLUDE_DIRS "")

target_link_libraries(main PRIVATE GSL etl)