# Core library: problem model, energy model and search
add_library(ev_vrptw_core STATIC
    src/Instance.cpp
    src/LNS.cpp
    src/LNSar.cpp
    src/MST.cpp
    src/Route.cpp
    src/SubStep.cpp
)

target_include_directories(ev_vrptw_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(ev_vrptw_core PUBLIC /W4)
else()
    target_compile_options(ev_vrptw_core PUBLIC -Wall -Wextra)
endif()

# Static runtime on MinGW so the exe runs outside the MinGW shell
if(MINGW)
    target_link_options(ev_vrptw_core PUBLIC -static)
endif()

# Experiment driver
add_executable(ev_vrptw
    experiments/main.cpp
    experiments/exp0.cpp
    experiments/exp1.cpp
    experiments/exp2.cpp
)

target_include_directories(ev_vrptw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/experiments)
target_link_libraries(ev_vrptw PRIVATE ev_vrptw_core)

# Default data/results roots (overridable at runtime: --data/--results or EV_VRPTW_DATA/EV_VRPTW_RESULTS)
target_compile_definitions(ev_vrptw PRIVATE
    EV_VRPTW_DEFAULT_DATA_DIR="${PROJECT_SOURCE_DIR}/data"
    EV_VRPTW_DEFAULT_RESULTS_DIR="${PROJECT_SOURCE_DIR}/results"
)
