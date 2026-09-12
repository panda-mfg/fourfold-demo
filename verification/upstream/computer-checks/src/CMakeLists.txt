# Common 
add_library(project_deps INTERFACE)
target_link_libraries(project_deps INTERFACE
  Boost::boost
  Boost::program_options
  Boost::thread
  spdlog::spdlog
  fmt::fmt
)
target_compile_options(project_deps INTERFACE
  -O2 -Wall
)
target_compile_features(project_deps INTERFACE
  cxx_std_23
)

# lib
add_library(combine_lib STATIC
  cartwheel.cpp
  combine_cartwheel.cpp
  configuration.cpp
  degree.cpp
  mapping.cpp
  pseudo_configuration.cpp
  pseudo_triangulation.cpp
  rule.cpp
)
target_link_libraries(combine_lib PUBLIC
  project_deps
)

# main
add_executable(main
  main.cpp
)
target_link_libraries(main PRIVATE 
  project_deps
  combine_lib
)
