#ifndef LLVM_CLANG_LIB_CODEGEN_ALTENTRYMETA_H
#define LLVM_CLANG_LIB_CODEGEN_ALTENTRYMETA_H

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/JSON.h"

class AltEntryPoint {
public:
  std::string originalFunctionName;
  std::string filename;
  // Constructor
  AltEntryPoint(std::string originalFunctionName, std::string filename)
      : originalFunctionName(originalFunctionName), filename(filename) {}
};

class AltEntryPointMeta {
public:
  std::vector<AltEntryPoint> entryPoints;
  AltEntryPointMeta() {}

  void push_back(AltEntryPoint entryPoint) {
    entryPoints.push_back(entryPoint);
  }

  void write(llvm::json::OStream &J) {
    J.object([&]() {
      J.attributeArray("entry_points", [&]() {
        for (auto &e : entryPoints) {
          J.object([&]() {
            J.attribute("original_function_name", e.originalFunctionName);
            J.attribute("filename", e.filename);
          });
        }
      });
    });
  }

  void read(llvm::json::Value &MetaJSON) {
    entryPoints.clear();

    llvm::json::Object *O = MetaJSON.getAsObject();
    if (O == nullptr) {
      llvm::errs() << "Failed to parse: (top level is not a object)\n";
      return;
    }
    // Get the array of entry points
    llvm::json::Array *A = O->getArray("entry_points");
    if (A == nullptr) {
      llvm::errs() << "Failed to parse: (entry_points missing)\n";
      return;
    }

    // Iterate over the entry points
    for (auto E : *A) {
      llvm::json::Object *Entry = E.getAsObject();
      if (Entry == nullptr) {
        llvm::errs()
            << "Failed to parse: (entry_points element is not an object)\n";
        return;
      }

      // Get the filename of the entry point
      llvm::Optional<llvm::StringRef> EFilename = Entry->getString("filename");
      if (!EFilename.hasValue()) {
        llvm::errs()
            << "Failed to parse: (entry_points element has no filename)\n";
        return;
      }

      // Get the filename of the entry point
      llvm::Optional<llvm::StringRef> EOriginalFunctionName =
          Entry->getString("original_function_name");
      if (!EOriginalFunctionName.hasValue()) {
        llvm::errs() << "Failed to parse: (entry_points element has no "
                        "original_function_name)\n";
        return;
      }

      entryPoints.push_back(AltEntryPoint(
          EOriginalFunctionName.getValue().str(), EFilename.getValue().str()));
    }
  }
};

#endif