set(LLVM_LINK_COMPONENTS support)

add_subdirectory(loop_trans)

add_clang_executable(loop-convert
  LoopConvert.cpp
  )

target_include_directories(loop-convert PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/loop_trans"
  )



target_link_libraries(loop-convert
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  loop_trans
  )




