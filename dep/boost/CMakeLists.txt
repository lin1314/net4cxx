
find_package(Boost 1.65 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup log
        date_time stacktrace_basic)

add_library(boost INTERFACE)

target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})

target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})