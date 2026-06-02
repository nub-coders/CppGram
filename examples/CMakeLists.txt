# examples/CMakeLists.txt  (rename to CMakeLists.txt inside examples/)
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE CppGram::cppgram)

add_executable(test_bot test_bot.cpp)
target_link_libraries(test_bot PRIVATE CppGram::cppgram)