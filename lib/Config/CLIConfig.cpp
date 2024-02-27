//===- CLIConfigBuilder.cpp - QEConfig from the CLI ------*- C++ -*-------===//
//
// (C) Copyright IBM 2023, 2024.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
///
///  This file implements building the configuration from the CLI.
///
//===----------------------------------------------------------------------===//

#include "Config/CLIConfig.h"
#include "Config/QEConfig.h"

#include "mlir/Debug/CLOptionsSetup.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace qec::config;

namespace {
// The space below at the front of the string causes this category to be printed
// first
llvm::cl::OptionCategory
    qecCat_(" qe-compiler options",
             "Options that control high-level behavior of QE Compiler");

llvm::cl::OptionCategory
    optCat_(" qe-compiler options: opt",
            "Options that control behaviour inherited from mlir-opt.");

class BytecodeVersionParser : public llvm::cl::parser<std::optional<int64_t>> {
public:
  BytecodeVersionParser(llvm::cl::Option &O)
      : llvm::cl::parser<std::optional<int64_t>>(O) {}

  bool parse(llvm::cl::Option &O, llvm::StringRef /*argName*/,
             llvm::StringRef arg, std::optional<int64_t> &v) {
    long long w;
    if (getAsSignedInteger(arg, 10, w))
      return O.error("Invalid argument '" + arg +
                     "', only integer is supported.");
    v = w;
    return false;
  }
};

/// This class is intended to manage the handling of command line options for
/// creating a qe-compiler mlir-opt based config. This is a singleton.
/// The implementation closely follows that of
/// https://github.com/llvm/llvm-project/blob/llvmorg-17.0.6/mlir/lib/Tools/mlir-opt/MlirOptMain.cpp
/// As the implementation is anonymous we recreate the population of the
/// configuration here.
struct QEConfigCLOptions : public QEConfig {
  QEConfigCLOptions() {

    static llvm::cl::opt<enum InputType, /*ExternalStorage=*/true> const
        inputType_(
            "X", llvm::cl::location(inputType),
            llvm::cl::desc("Specify the kind of input desired"),
            llvm::cl::values(
                clEnumValN(InputType::QASM, "qasm",
                           "load the input file as an OpenQASM 3.0 source")),
            llvm::cl::values(clEnumValN(InputType::MLIR, "mlir",
                                        "load the input file as an MLIR file")),
            llvm::cl::values(
                clEnumValN(InputType::Bytecode, "bytecode",
                           "load the input file as an MLIR bytecode file - "
                           "equivalent to -X=mlir as MLIR treats bytecode as "
                           "valid MLIR during parsing.")));

    static llvm::cl::opt<enum EmitAction, /*ExternalStorage=*/true> const
        emitAction_(
            "emit", llvm::cl::location(emitAction),
            llvm::cl::desc("Select the kind of output desired"),
            llvm::cl::values(
                clEnumValN(EmitAction::AST, "ast", "output the AST dump")),
            llvm::cl::values(clEnumValN(EmitAction::ASTPretty, "ast-pretty",
                                        "pretty print the AST")),
            llvm::cl::values(clEnumValN(EmitAction::MLIR, "mlir",
                                        "output MLIR textual format")),
            llvm::cl::values(clEnumValN(EmitAction::Bytecode, "bytecode",
                                        "output MLIR bytecode")),
            llvm::cl::values(clEnumValN(EmitAction::WaveMem, "wavemem",
                                        "output the waveform memory")),
            llvm::cl::values(
                clEnumValN(EmitAction::QEM, "qem",
                           "generate a quantum executable module (qem) "
                           "for execution on hardware")),
            llvm::cl::values(clEnumValN(
                EmitAction::QEQEM, "qe-qem",
                "generate a target-specific quantum executable module (qeqem) "
                "for execution on hardware")),
            llvm::cl::values(
                clEnumValN(EmitAction::None, "none", "output nothing")));

    static llvm::cl::opt<std::string> targetConfigPath_(
        "config",
        llvm::cl::desc(
            "Path to configuration file or directory (depends on the "
            "target), - means use the config service"),
        llvm::cl::value_desc("path"), llvm::cl::cat(getQECCLCategory()));

    targetConfigPath_.setCallback([&](const std::string &config) {
      if (config != "")
        targetConfigPath = config;
    });

    static llvm::cl::opt<std::string> targetName_(
        "target",
        llvm::cl::desc(
            "Target architecture. Required for machine code generation."),
        llvm::cl::value_desc("targetName"), llvm::cl::cat(getQECCLCategory()));

    targetName_.setCallback([&](const std::string &target) {
      if (target != "")
        targetName = target;
    });

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const addTargetPasses(
        "add-target-passes", llvm::cl::desc("Add target-specific passes"),
        llvm::cl::location(addTargetPassesFlag), llvm::cl::init(true),
        llvm::cl::cat(getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const showTargets(
        "show-targets", llvm::cl::desc("Print the list of registered targets"),
        llvm::cl::location(showTargetsFlag), llvm::cl::init(false),
        llvm::cl::cat(qec::config::getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const showPayloads(
        "show-payloads",
        llvm::cl::desc("Print the list of registered payloads"),
        llvm::cl::location(showPayloadsFlag), llvm::cl::init(false),
        llvm::cl::cat(qec::config::getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const showConfig(
        "show-config",
        llvm::cl::desc("Print the loaded compiler configuration."),
        llvm::cl::location(showConfigFlag), llvm::cl::init(false),
        llvm::cl::cat(qec::config::getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const plaintextPayload(
        "plaintext-payload", llvm::cl::desc("Write the payload in plaintext"),
        llvm::cl::location(emitPlaintextPayloadFlag), llvm::cl::init(false),
        llvm::cl::cat(qec::config::getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const includeSource(
        "include-source",
        llvm::cl::desc("Write the input source into the payload"),
        llvm::cl::location(includeSourceFlag), llvm::cl::init(false),
        llvm::cl::cat(qec::config::getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const compileTargetIr(
        "compile-target-ir",
        llvm::cl::desc("Apply the target's IR compilation"),
        llvm::cl::location(compileTargetIRFlag), llvm::cl::init(false),
        llvm::cl::cat(qec::config::getQECCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const
        bypassPayloadTargetCompilation(
            "bypass-payload-target-compilation",
            llvm::cl::desc(
                "Bypass target compilation during payload generation."),
            llvm::cl::location(bypassPayloadTargetCompilationFlag),
            llvm::cl::init(false),
            llvm::cl::cat(qec::config::getQECCLCategory()));

    // mlir-opt options

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const
        allowUnregisteredDialects(
            "allow-unregistered-dialect",
            llvm::cl::desc("Allow operation with no registered dialects"),
            llvm::cl::location(allowUnregisteredDialectsFlag),
            llvm::cl::init(false), llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const dumpPassPipeline(
        "dump-pass-pipeline",
        llvm::cl::desc("Print the pipeline that will be run"),
        llvm::cl::location(dumpPassPipelineFlag), llvm::cl::init(false),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<std::optional<int64_t>, /*ExternalStorage=*/true,
                         BytecodeVersionParser> const
        bytecodeVersion(
            "emit-bytecode-version",
            llvm::cl::desc("Use specified bytecode when generating output"),
            llvm::cl::location(emitBytecodeVersion),
            llvm::cl::init(std::nullopt), llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<std::string, /*ExternalStorage=*/true> const irdlFile(
        "irdl-file",
        llvm::cl::desc("IRDL file to register before processing the input"),
        llvm::cl::location(irdlFileFlag), llvm::cl::init(""),
        llvm::cl::value_desc("filename"), llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const
        enableDebuggerHook(
            "mlir-enable-debugger-hook",
            llvm::cl::desc("Enable Debugger hook for debugging MLIR Actions"),
            llvm::cl::location(enableDebuggerActionHookFlag),
            llvm::cl::init(false), llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const explicitModule(
        "no-implicit-module",
        llvm::cl::desc(
            "Disable implicit addition of a top-level module op during "
            "parsing"),
        llvm::cl::location(useExplicitModuleFlag), llvm::cl::init(false),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const runReproducer(
        "run-reproducer",
        llvm::cl::desc("Run the pipeline stored in the reproducer"),
        llvm::cl::location(runReproducerFlag), llvm::cl::init(false),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const showDialects(
        "show-dialects",
        llvm::cl::desc("Print the list of registered dialects and exit"),
        llvm::cl::location(showDialectsFlag), llvm::cl::init(false),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const splitInputFile(
        "split-input-file",
        llvm::cl::desc("Split the input file into pieces and process each "
                       "chunk independently"),
        llvm::cl::location(splitInputFileFlag), llvm::cl::init(false),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const
        verifyDiagnostics(
            "verify-diagnostics",
            llvm::cl::desc("Check that emitted diagnostics match "
                           "expected-* lines on the corresponding line"),
            llvm::cl::location(verifyDiagnosticsFlag), llvm::cl::init(false),
            llvm::cl::cat(getQEOptCLCategory()));

#ifndef NOVERIFY
#define VERIFY_PASSES_DEFAULT true
#else
#define VERIFY_PASSES_DEFAULT false
#endif
    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const verifyPasses(
        "verify-each",
        llvm::cl::desc("Run the verifier after each transformation pass"),
        llvm::cl::location(verifyPassesFlag),
        llvm::cl::init(VERIFY_PASSES_DEFAULT),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::opt<bool, /*ExternalStorage=*/true> const verifyRoundtrip(
        "verify-roundtrip",
        llvm::cl::desc(
            "Round-trip the IR after parsing and ensure it succeeds"),
        llvm::cl::location(verifyRoundtripFlag), llvm::cl::init(false),
        llvm::cl::cat(getQEOptCLCategory()));

    static llvm::cl::list<std::string> passPlugins_(
        "load-pass-plugin",
        llvm::cl::desc("Load passes from plugin library. It is required that "
                       "the pass be specified to be loaded before all usages "
                       "of dynamic CL arguments."),
        llvm::cl::cat(getQEOptCLCategory()));
    /// Set the callback to load a pass plugin.
    passPlugins_.setCallback([&](const std::string &pluginPath) {
      passPlugins.push_back(pluginPath);
      if (mlir::failed(loadPassPlugin(pluginPath)))
        llvm::errs() << "Failed to load passes from '" << pluginPath
                     << "'. Request ignored.\n";
    });

    static llvm::cl::list<std::string> dialectPlugins_(
        "load-dialect-plugin",
        llvm::cl::desc("Load dialects from plugin library. It is required that "
                       "the dialect be specified to be loaded before all "
                       "usages of dynamic CL arguments"),
        llvm::cl::cat(getQEOptCLCategory()));
    this->dialectPlugins_ = std::addressof(dialectPlugins_);

    static mlir::PassPipelineCLParser const passPipeline(
        "", "Compiler passes to run", "p");
    setPassPipelineParser(passPipeline);

    static llvm::cl::opt<enum QEVerbosity, /*ExternalStorage=*/true> const
        verbosity(
            "verbosity", llvm::cl::location(verbosityLevel),
            llvm::cl::init(QEVerbosity::_VerbosityCnt),
            llvm::cl::desc("Set verbosity level for output, default is warn"),
            llvm::cl::values(
                clEnumValN(QEVerbosity::Error, "error", "Emit only errors")),
            llvm::cl::values(
                clEnumValN(QEVerbosity::Warn, "warn", "Also emit warnings")),
            llvm::cl::values(clEnumValN(QEVerbosity::Info, "info",
                                        "Also emit informational messages")),
            llvm::cl::values(clEnumValN(QEVerbosity::Debug, "debug",
                                        "Also emit debug messages")),
            llvm::cl::cat(qec::config::getQEOptCLCategory()));

    static llvm::cl::opt<int> maxThreads_(
        "max-threads",
        llvm::cl::desc(
            "Set the maximum number of threads for the MLIR context."),
        llvm::cl::init(-1));

    maxThreads_.setCallback([&](const int &cliMaxThreads) {
      if (cliMaxThreads > 0)
        maxThreads = cliMaxThreads;
    });
  }

  /// Pointer to static dialectPlugins variable in constructor, needed by
  /// setDialectPluginsCallback(DialectRegistry&).
  llvm::cl::list<std::string> *dialectPlugins_ = nullptr;

  void setDialectPluginsCallback(mlir::DialectRegistry &registry) {
    dialectPlugins_->setCallback([&](const std::string &pluginPath) {
      dialectPlugins.push_back(pluginPath);
      if (mlir::failed(loadDialectPlugin(pluginPath, registry)))
        llvm::errs() << "Failed to load dialect from '" << pluginPath
                     << "'. Request ignored.\n";
    });
  }

  llvm::Error computeInputType(llvm::StringRef inputFilename) {
    if (getInputType() == InputType::Undetected) {

      // Override with mlir opt config if set (it typically shouldn't be)
      if (shouldEmitBytecode())
        setInputType(InputType::Bytecode);
      else
        setInputType(fileExtensionToInputType(getExtension(inputFilename)));
      if (inputFilename != "-" && getInputType() == InputType::Undetected) {
        return llvm::createStringError(
            llvm::inconvertibleErrorCode(),
            "Unable to autodetect file extension type! Please specify the "
            "input type with -X");
      }
    }

    return llvm::Error::success();
  }

  llvm::Error computeOutputType(llvm::StringRef outputFilename) {
    if (outputFilename != "-") {
      EmitAction const extensionAction =
          fileExtensionToAction(getExtension(outputFilename));
      if (extensionAction == EmitAction::Undetected &&
          emitAction == EmitAction::Undetected) {
        llvm::errs() << "Cannot determine the file extension of the specified "
                        "output file "
                     << outputFilename << " defaulting to dumping MLIR\n";
        setEmitAction(EmitAction::MLIR);
      } else if (emitAction == EmitAction::Undetected) {
        setEmitAction(extensionAction);
      } else if (extensionAction != getEmitAction()) {
        llvm::errs()
            << "Warning! The output type in the file extension doesn't "
               "match the output type specified by --emit!\n";
      }
    } else {
      if (emitAction == EmitAction::Undetected)
        setEmitAction(EmitAction::MLIR);
    }

    return llvm::Error::success();
  }
};

} // anonymous namespace

llvm::ManagedStatic<QEConfigCLOptions> clOptionsConfig;

llvm::cl::OptionCategory &qec::config::getQECCLCategory() { return qecCat_; }
llvm::cl::OptionCategory &qec::config::getQEOptCLCategory() {
  return optCat_;
}

CLIConfigBuilder::CLIConfigBuilder() {
  clOptionsConfig->setDebugConfig(
      mlir::tracing::DebugConfig::createFromCLOptions());
}

void CLIConfigBuilder::registerCLOptions(mlir::DialectRegistry &registry) {
  clOptionsConfig->setDialectPluginsCallback(registry);
  mlir::tracing::DebugConfig::registerCLOptions();
}

llvm::Error CLIConfigBuilder::populateConfig(QEConfig &config) {

  config.setDebugConfig(clOptionsConfig->getDebugConfig());

  config.setPassPipelineSetupFn(clOptionsConfig->passPipelineCallback);

  if (clOptionsConfig->verbosityLevel != QEVerbosity::_VerbosityCnt)
    config.verbosityLevel = clOptionsConfig->verbosityLevel;

  // qe
  if (clOptionsConfig->targetName.has_value())
    config.targetName = clOptionsConfig->targetName;
  if (clOptionsConfig->targetConfigPath.has_value())
    config.targetConfigPath = clOptionsConfig->targetConfigPath;

  config.addTargetPassesFlag = clOptionsConfig->addTargetPassesFlag;
  config.showTargetsFlag = clOptionsConfig->showTargetsFlag;
  config.showPayloadsFlag = clOptionsConfig->showPayloadsFlag;
  config.showConfigFlag = clOptionsConfig->showConfigFlag;
  config.emitPlaintextPayloadFlag = clOptionsConfig->emitPlaintextPayloadFlag;
  config.includeSourceFlag = clOptionsConfig->includeSourceFlag;
  config.compileTargetIRFlag = clOptionsConfig->compileTargetIRFlag;
  config.bypassPayloadTargetCompilationFlag =
      clOptionsConfig->bypassPayloadTargetCompilationFlag;
  config.passPlugins.insert(config.passPlugins.end(),
                            clOptionsConfig->passPlugins.begin(),
                            clOptionsConfig->passPlugins.end());
  config.dialectPlugins.insert(config.dialectPlugins.end(),
                               clOptionsConfig->dialectPlugins.begin(),
                               clOptionsConfig->dialectPlugins.end());

  if (clOptionsConfig->maxThreads.has_value())
    config.maxThreads = clOptionsConfig->maxThreads;

  // opt
  config.allowUnregisteredDialectsFlag =
      clOptionsConfig->allowUnregisteredDialectsFlag;
  config.dumpPassPipelineFlag = clOptionsConfig->dumpPassPipelineFlag;
  if (clOptionsConfig->emitBytecodeVersion.has_value())
    config.emitBytecodeVersion = clOptionsConfig->emitBytecodeVersion;
  config.irdlFileFlag = clOptionsConfig->irdlFileFlag;
  config.enableDebuggerActionHookFlag =
      clOptionsConfig->enableDebuggerActionHookFlag;
  config.useExplicitModuleFlag = clOptionsConfig->useExplicitModuleFlag;
  config.runReproducerFlag = clOptionsConfig->runReproducerFlag;
  config.showDialectsFlag = clOptionsConfig->showDialectsFlag;
  config.verifyDiagnosticsFlag = clOptionsConfig->verifyDiagnosticsFlag;
  config.verifyPassesFlag = clOptionsConfig->verifyPassesFlag;
  config.verifyRoundtripFlag = clOptionsConfig->verifyRoundtripFlag;
  config.splitInputFileFlag = clOptionsConfig->splitInputFileFlag;
  return llvm::Error::success();
}

llvm::Error CLIConfigBuilder::populateConfig(QEConfig &config,
                                             llvm::StringRef inputFilename,
                                             llvm::StringRef outputFilename) {

  if (auto err = populateConfig(config))
    return err;

  if (auto err = clOptionsConfig->computeInputType(inputFilename))
    return err;
  config.inputType = clOptionsConfig->inputType;

  if (auto err = clOptionsConfig->computeOutputType(outputFilename))
    return err;
  config.emitAction = clOptionsConfig->emitAction;

  return llvm::Error::success();
}
