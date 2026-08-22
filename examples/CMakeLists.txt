# examples/CMakeLists.txt
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE CppGram::cppgram)

add_executable(test_bot test_bot.cpp)
target_link_libraries(test_bot PRIVATE CppGram::cppgram)

add_executable(v02_features_demo v02_features_demo.cpp)
target_link_libraries(v02_features_demo PRIVATE CppGram::cppgram)

add_executable(v03_features_demo v03_features_demo.cpp)
target_link_libraries(v03_features_demo PRIVATE CppGram::cppgram)

add_executable(v04_features_demo v04_features_demo.cpp)
target_link_libraries(v04_features_demo PRIVATE CppGram::cppgram)

add_executable(v05_features_demo v05_features_demo.cpp)
target_link_libraries(v05_features_demo PRIVATE CppGram::cppgram)

add_executable(interactive_cli interactive_cli.cpp)
target_link_libraries(interactive_cli PRIVATE CppGram::cppgram)

