#include "CpplessLd.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Options.h"
#include "llvm/Option/ArgList.h"
#include "clang/Driver/Driver.h"

using namespace clang::driver::tools::cppless;
using namespace llvm::opt;
using namespace clang;

void Linker::ConstructJob(Compilation &C, const JobAction &JA,
                          const InputInfo &Output, const InputInfoList &Inputs,
                          const llvm::opt::ArgList &Args,
                          const char *LinkingOutput) const {
  // Relay to ourselves
  SmallVector<const char *, 16> newArgv;
  auto &args = C.getInputArgs();
  args.ClaimAllArgs();
  for (auto arg : args) {
    if (arg->getOption().matches(options::OPT_INPUT))
      continue;
    if (arg->getOption().matches(options::OPT_o))
      continue;
    if (arg->getOption().matches(options::OPT_c))
      continue;
    arg->render(args, newArgv);
  }
  newArgv.push_back("-cppless-driver-path");
  newArgv.push_back(C.getDriver().getClangProgramPath());
  for (const auto &II : Inputs) {
    // Ignore for now
    if (!II.isFilename())
      continue;
    newArgv.push_back("-cppless-input");
    newArgv.push_back(II.getFilename());
  }
  newArgv.push_back("-cppless-output");
  newArgv.push_back(Output.getFilename());
  const char *Exec =
      Args.MakeArgString(getToolChain().GetProgramPath("cppless-ld"));
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::None(),
                                         Exec, newArgv, Inputs, Output));
}