set(LLVM_LINK_COMPONENTS
  Core
  Support
  )

add_clang_tool(cppless-ld
  CpplessLd.cpp
  )

set(CLANG_SCAN_DEPS_LIB_DEPS
  )

clang_target_link_libraries(cppless-ld
  PRIVATE
  ${CLANG_SCAN_DEPS_LIB_DEPS}
  )

