set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

add_clang_executable(clang-mytool
  MyTool.cpp
  FindLoggingFunction.cpp
  FindLoggedSnippet.cpp
  )

target_link_libraries(clang-mytool
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangStaticAnalyzerFrontend
  clangTooling
  )

install(TARGETS clang-mytool
  RUNTIME DESTINATION bin)
