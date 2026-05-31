# tests/CMakeLists.txt  (rename to CMakeLists.txt inside tests/)
# Phase 1 smoke test — GoogleTest is wired up in Phase 9.
add_executable(phase1_smoke phase1_smoke.cpp)
target_link_libraries(phase1_smoke PRIVATE CppGram::cppgram)
add_test(NAME phase1_smoke COMMAND phase1_smoke)