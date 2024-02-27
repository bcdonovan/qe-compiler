//===- TargetSystemInfo.h - System Target Registry --------------*- C++ -*-===//
//
// (C) Copyright IBM 2023.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
//
//  Declaration of the QEC target system info.
//
//===----------------------------------------------------------------------===//
#ifndef TARGETSYSTEMINFO_H
#define TARGETSYSTEMINFO_H

#include "Arguments/Arguments.h"
#include "HAL/TargetSystem.h"
#include "Plugin/PluginInfo.h"
#include "Support/Pimpl.h"

namespace qec::hal::registry {

/// Class to group info about a registered target. Such as how to invoke
/// and a description.
class TargetSystemInfo
    : public qec::plugin::registry::PluginInfo<qec::hal::TargetSystem> {
  using PluginInfo =
      qec::plugin::registry::PluginInfo<qec::hal::TargetSystem>;
  using PassesFunction = std::function<llvm::Error()>;
  using PassPipelinesFunction = std::function<llvm::Error()>;

public:
  /// Construct this entry
  TargetSystemInfo(llvm::StringRef name, llvm::StringRef description,
                   PluginInfo::PluginFactoryFunction targetFactory,
                   PassesFunction passRegistrar,
                   PassPipelinesFunction passPipelineRegistrar);

  ~TargetSystemInfo();

  /// Create the target system and register it under the given context.
  llvm::Expected<qec::hal::TargetSystem *>
  createTarget(mlir::MLIRContext *context,
               std::optional<PluginInfo::PluginConfiguration> configuration);

  /// Get the target system registered for the given context. First checks for
  /// a target registered exactly for the given context. If no such context is
  /// found, checks if a target is registered under nullptr, and returns
  /// that. If no target is found, an error is returned.
  llvm::Expected<qec::hal::TargetSystem *>
  getTarget(mlir::MLIRContext *context) const;

  /// Register this target's MLIR passes with the QEC system.
  /// Should only be called once on initialization.
  llvm::Error registerTargetPasses() const;

  /// Register this target's MLIR passe pipelines with the QEC system.
  /// Should only be called once on initialization.
  llvm::Error registerTargetPassPipelines() const;

private:
  struct Impl;

  qec::support::Pimpl<Impl> impl;

  PassesFunction passRegistrar;

  PassPipelinesFunction passPipelineRegistrar;
};

} // namespace qec::hal::registry

#endif
