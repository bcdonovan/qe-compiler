//===- QEC.cpp -------------------------------------------------*- C++ -*-===//
//
// (C) Copyright IBM 2023.
//
// This code is part of Qiskit.
//
// This code is licensed under the Apache License, Version 2.0 with LLVM
// Exceptions. You may obtain a copy of this license in the LICENSE.txt
// file in the root directory of this source tree.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements QEC.
///
//===----------------------------------------------------------------------===//

#include "QEC.h"

#include "Config.h"
#include "HAL/TargetSystem.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"

#include <cstdlib>
#include <string>

using namespace qec;
using namespace llvm;

#define EXPORT_VERSION_STRING(FN, STR)                                         \
  llvm::StringRef qec::FN() {                                                  \
    static const char *versionString = STR;                                    \
    return versionString;                                                      \
  }

EXPORT_VERSION_STRING(getQECMajorVersion, QEC_VERSION_MAJOR)
EXPORT_VERSION_STRING(getQECMinorVersion, QEC_VERSION_MINOR)
EXPORT_VERSION_STRING(getQECPatchlevel, QEC_VERSION_PATCH)
EXPORT_VERSION_STRING(getQECVersion, QEC_VERSION)

#undef EXPORT_VERSION_STRING

namespace {
llvm::StringRef _getResourcesDir() {
  if (char *env = getenv("QEC_RESOURCES")) {
    /* strings returned by getenv may be invalidated, so keep a copy */
    static std::string const resourcesDir{env};
    return resourcesDir;
  }

  /* fallback to compiled-in installation path */
  return QEC_RESOURCES_INSTALL_PREFIX;
}

}; // namespace

llvm::StringRef qec::getResourcesDir() {
  static llvm::StringRef const resourcesDir = _getResourcesDir();
  return resourcesDir;
}

llvm::SmallString<128>
qec::getTargetResourcesDir(qec::hal::Target const *target) {
  // target-specific resources are at path "targets/<name of target>" below the
  // resource directory
  llvm::SmallString<128> path(getResourcesDir());

  llvm::sys::path::append(path, "targets", target->getResourcePath());
  return path;
}
