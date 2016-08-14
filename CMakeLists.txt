set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

add_clang_executable(clang-smartlog
  src/SmartLog.cpp
  src/FindLoggingFunction.cpp
  src/FindLoggedSnippet.cpp
  src/FindOutputSnippet.cpp
  src/FindPatternSnippet.cpp
  src/FindKeywordFunction.cpp
  src/LogTimes.cpp
  src/InsertLog.cpp
  )

target_link_libraries(clang-smartlog
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangStaticAnalyzerFrontend
  clangTooling
  )

install(TARGETS clang-smartlog
  RUNTIME DESTINATION bin)
