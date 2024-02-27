//===- CLIConfig.h - CLI Configuration builder ------------------*- C++ -*-===//
//
// (C) Copyright IBM 2023, 2024.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
///
///  Populate the configuration from the CLI.
///
//===----------------------------------------------------------------------===//
#ifndef QEC_CLICONFIG_H
#define QEC_CLICONFIG_H

#include "Config/QEConfig.h"

#include "llvm/Support/CommandLine.h"

namespace qec::config {

/// @brief Get the CLI category for the QE compiler.
/// @return The reference to the CLI category for the compiler.
llvm::cl::OptionCategory &getQECCLCategory();

/// @brief Get the CLI category for the QE compiler mlir-opt options.
/// @return The reference to the CLI category for the compiler.
llvm::cl::OptionCategory &getQEOptCLCategory();

/// @brief Build a QEConfig from input CLI arguments.
///
/// When the compiler is invoked it loads the CLI
/// using the MLIR/LLVM CLI library. This enables the
/// inheritance of all of MLIR's powerful CLI functionality.
///
/// The qe-compiler adds several cli arguments to
/// configure the QEConfig through the CLIConfigBuilder.
class CLIConfigBuilder : public QEConfigBuilder {
public:
  explicit CLIConfigBuilder();
  static void registerCLOptions(mlir::DialectRegistry &registry);
  llvm::Error populateConfig(QEConfig &config) override;
  llvm::Error populateConfig(QEConfig &config, llvm::StringRef inputFilename,
                             llvm::StringRef outputFilename);
};

} // namespace qec::config
#endif // QE_CLICONFIG_H
