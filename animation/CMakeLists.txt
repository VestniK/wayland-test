add_executable(animation main.cpp)
target_link_libraries(animation PRIVATE wlpp io CONAN_PKG::gsl_microsoft)
