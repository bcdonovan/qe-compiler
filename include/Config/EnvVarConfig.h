//===- EnvVarConfig.h - EnvVar Configuration builder ----------*- C++-*----===//
//
// (C) Copyright IBM 2023, 2024.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
///
///  Populate the configuration from environment variables.
///
//===----------------------------------------------------------------------===//
#ifndef QEC_ENVVARCONFIG_H
#define QEC_ENVVARCONFIG_H

#include "Config/QEConfig.h"

namespace qec::config {

/// @brief Populate arguments of the QEConfig
/// from environment variables.
///
///
/// The qe-compiler makes several several QEConfig configuration
/// options configurable from environment variables through the
/// EnvVarConfigBuilder.
///
/// These currently are:
/// - `QEC_TARGET_NAME`: Sets QEConfig::targetName.
/// - `QEC_TARGET_CONFIG_PATH`: Sets QEConfig::targetConfigPath.
/// - `QEC_VERBOSITY`: Set the compiler output verbosity. One of
/// "ERROR/WARN/INFO/DEBUG".
/// - `QEC_MAX_THREADS`: Sets the maximum number of compiler threads when
/// initializing the MLIR context's threadpool.
///
class EnvVarConfigBuilder : public QEConfigBuilder {
public:
  llvm::Error populateConfig(QEConfig &config) override;

private:
  llvm::Error populateConfigurationPath_(QEConfig &config);
  llvm::Error populateTarget_(QEConfig &config);
  llvm::Error populateVerbosity_(QEConfig &config);
  llvm::Error populateMaxThreads_(QEConfig &config);
};

} // namespace qec::config
#endif // QE_ENVVARCONFIG_H
