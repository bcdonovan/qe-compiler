//===- EnvVarConfigBuilder.cpp - QEConfig from EnvVars  ----* C++*--------===//
//
// (C) Copyright IBM 2023, 2024.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
//
//  This file implements building the configuration from environemnt variables.
//
//===----------------------------------------------------------------------===//

#include "Config/EnvVarConfig.h"
#include "Config/QEConfig.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstdlib>
#include <cstring>

using namespace qec::config;

llvm::Error EnvVarConfigBuilder::populateConfig(QEConfig &config) {
  if (auto err = populateConfigurationPath_(config))
    return err;

  if (auto err = populateTarget_(config))
    return err;

  if (auto err = populateVerbosity_(config))
    return err;

  if (auto err = populateMaxThreads_(config))
    return err;

  return llvm::Error::success();
}

llvm::Error EnvVarConfigBuilder::populateConfigurationPath_(QEConfig &config) {
  if (const char *configurationPath = std::getenv("QEC_TARGET_CONFIG_PATH"))
    config.targetConfigPath = configurationPath;
  return llvm::Error::success();
}

llvm::Error EnvVarConfigBuilder::populateTarget_(QEConfig &config) {
  if (const char *targetStr = std::getenv("QEC_TARGET_NAME"))
    config.targetName = targetStr;
  return llvm::Error::success();
}

llvm::Error EnvVarConfigBuilder::populateMaxThreads_(QEConfig &config) {
  if (const char *maxThreads = std::getenv("QEC_MAX_THREADS")) {
    llvm::StringRef maxThreadsStr(maxThreads);
    unsigned int maxThreadsInt;
    if (maxThreadsStr.consumeInteger<unsigned int>(0, maxThreadsInt))
      return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                     "Unable to parse maximum threads from \"" +
                                         maxThreadsStr + "\"\n");

    config.maxThreads = maxThreadsInt;
  }

  return llvm::Error::success();
}

llvm::Error EnvVarConfigBuilder::populateVerbosity_(QEConfig &config) {
  if (const char *verbosity = std::getenv("QEC_VERBOSITY")) {
    if (strcmp(verbosity, "ERROR") == 0) {
      config.setVerbosityLevel(QEVerbosity::Error);
    } else if (strcmp(verbosity, "WARN") == 0) {
      config.setVerbosityLevel(QEVerbosity::Warn);
    } else if (strcmp(verbosity, "INFO") == 0) {
      config.setVerbosityLevel(QEVerbosity::Info);
    } else if (strcmp(verbosity, "DEBUG") == 0) {
      config.setVerbosityLevel(QEVerbosity::Debug);
    } else {
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          "QEC_VERBOSITY level unrecognized got (" +
              llvm::StringRef(verbosity) +
              "), options are ERROR, WARN, INFO, or DEBUG\n");
    }
  }
  return llvm::Error::success();
}
