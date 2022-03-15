#ifndef LLVM_CLANG_LIB_DRIVER_TOOLS_CPPLESS_H
#define LLVM_CLANG_LIB_DRIVER_TOOLS_CPPLESS_H

#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace tools {

namespace cppless {

class LLVM_LIBRARY_VISIBILITY Linker : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("cppless::Linker", "linker", TC) {}

  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

} // namespace cppless

} // namespace tools
} // namespace driver
} // namespace clang

#endif