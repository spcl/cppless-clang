#include "../lib/CodeGen/AltEntryMeta.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include <iostream>
#include <string>
#include <vector>

std::string getJsonMetaPath(std::string ObjectPath) {
  llvm::SmallString<64U> SS(ObjectPath);
  llvm::sys::path::replace_extension(SS, ".json");
  return SS.c_str();
}

std::string getOutputName(std::string OutputName, int id) {
  auto Parent = llvm::sys::path::parent_path(OutputName);
  auto Stem = llvm::sys::path::stem(OutputName);
  auto Ext = llvm::sys::path::extension(OutputName);
  llvm::SmallString<128U> SS(Parent);
  llvm::sys::path::append(SS, Stem + "_alt_" + std::to_string(id) + Ext);
  return SS.c_str();
}

int main(int argc, char const *argv[]) {
  std::vector<std::string> OtherArgs;
  std::vector<std::string> InputFiles;
  std::string OutputFile;
  std::string DriverPath;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-cppless-driver-path") {
      DriverPath = argv[++i];
    } else if (arg == "-cppless-input") {
      InputFiles.push_back(argv[++i]);
    } else if (arg == "-cppless-output") {
      OutputFile = argv[++i];
    } else if (arg == "-falt-entry") {
      continue;
    } else if (arg == "-cppless") {
      continue;
    } else {
      OtherArgs.push_back(arg);
    }
  }

  int id = 0;
  int i = 0;
  for (auto InputFile : InputFiles) {
    auto JsonMetaPath = getJsonMetaPath(InputFile);
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> File =
        llvm::MemoryBuffer::getFile(JsonMetaPath);
    if (!File) {
      // If the file couldn't be read, assume it just doesn't exist.
      continue;
    }
    AltEntryPointMeta Meta;
    auto Buffer = File.get()->getBuffer();
    if (llvm::Expected<llvm::json::Value> MetaJSON =
            llvm::json::parse(Buffer)) {
      auto JSON = *MetaJSON;
      Meta.read(JSON);

      // Print
      for (auto E : Meta.entryPoints) {
        std::vector<llvm::StringRef> Args;
        for (auto &A : OtherArgs) {
          Args.push_back(A);
        }

        int j = 0;
        for (auto &I : InputFiles) {
          if (j == i) {
            Args.push_back(E.filename);
          } else {
            Args.push_back(I);
          }
          j++;
        }

        Args.push_back("-o");
        Args.push_back(getOutputName(OutputFile, id++));
        llvm::sys::ExecuteAndWait(DriverPath, Args);
      }
    }
    i++;
  }

  std::vector<llvm::StringRef> Args;
  for (auto &A : OtherArgs)
    Args.push_back(A);

  for (auto &I : InputFiles)
    Args.push_back(I);

  Args.push_back("-o");
  Args.push_back(OutputFile);
  llvm::sys::ExecuteAndWait(DriverPath, Args);

  return 0;
}
