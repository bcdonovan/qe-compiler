//===- errors.cpp  - Error reporting API ------------------------*- C++ -*-===//
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
///  This file implements the API for error reporting.
///
//===----------------------------------------------------------------------===//

#include "API/errors.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace {

std::string_view getErrorCategoryAsString(qec::ErrorCategory category) {
  using namespace qec;
  switch (category) {
  case ErrorCategory::OpenQASM3ParseFailure:
    return "OpenQASM 3 parse error";

  case ErrorCategory::QECompilerError:
    return "Unknown compiler error";

  case ErrorCategory::QECompilerNoInputError:
    return "Error when no input file or string is provided";

  case ErrorCategory::QECompilerCommunicationFailure:
    return "Error on compilation communication failure";

  case ErrorCategory::QECompilerEOFFailure:
    return "EOF Error";

  case ErrorCategory::QECompilerNonZeroStatus:
    return "Errored because non-zero status is returned";

  case ErrorCategory::QECompilerSequenceTooLong:
    return "Input sequence is too long";

  case ErrorCategory::QECompilationFailure:
    return "Failure during compilation";

  case ErrorCategory::QELinkerNotImplemented:
    return "BindArguments not implemented for target";

  case ErrorCategory::QELinkSignatureWarning:
    return "Signature file format is invalid but may be processed";

  case ErrorCategory::QELinkSignatureError:
    return "Signature file format is invalid";

  case ErrorCategory::QELinkAddressError:
    return "Signature address is invalid";

  case ErrorCategory::QELinkSignatureNotFound:
    return "Signature file not found";

  case ErrorCategory::QELinkArgumentNotFoundWarning:
    return "Parameter in signature not found in arguments";

  case ErrorCategory::QELinkInvalidPatchTypeError:
    return "Invalid patch point type";

  case ErrorCategory::QEControlSystemResourcesExceeded:
    return "Control system resources exceeded";

  case ErrorCategory::UncategorizedError:
    return "Compilation failure";
  }

  llvm_unreachable("unhandled category");
}

llvm::StringRef getSeverityAsString(qec::Severity sev) {
  switch (sev) {
  case qec::Severity::Info:
    return "Info";
  case qec::Severity::Warning:
    return "Warning";
  case qec::Severity::Error:
    return "Error";
  case qec::Severity::Fatal:
    return "Fatal";
  }

  llvm_unreachable("unhandled severity");
}

} // anonymous namespace

namespace qec {

std::string Diagnostic::toString() const {
  std::string str;
  llvm::raw_string_ostream ostream(str);

  ostream << getSeverityAsString(severity) << ": "
          << getErrorCategoryAsString(category) << "\n";
  ostream << message;

  return str;
}

llvm::Error emitDiagnostic(const OptDiagnosticCallback &onDiagnostic,
                           const Diagnostic &diag, std::error_code ec) {
  auto *diagnosticCallback =
      onDiagnostic.has_value() ? &onDiagnostic.value() : nullptr;
  if (diagnosticCallback)
    (*diagnosticCallback)(diag);
  return llvm::createStringError(ec, diag.toString());
}

llvm::Error emitDiagnostic(const OptDiagnosticCallback &onDiagnostic,
                           Severity severity, ErrorCategory category,
                           std::string message, std::error_code ec) {
  qec::Diagnostic const diag{severity, category, std::move(message)};
  return emitDiagnostic(onDiagnostic, diag, ec);
}

} // namespace qec
