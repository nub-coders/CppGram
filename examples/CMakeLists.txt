# examples/CMakeLists.txt  (rename to CMakeLists.txt inside examples/)
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE CppGram::cppgram)