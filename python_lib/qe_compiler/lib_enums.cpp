#include "lib_enums.h"
#include "errors.h"

#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace py = pybind11;

void addErrorCategory(py::module &m) {
  py::enum_<qec::ErrorCategory>(m, "ErrorCategory", py::arithmetic())
      .value("OpenQASM3ParseFailure",
             qec::ErrorCategory::OpenQASM3ParseFailure)
      .value("QECompilerError", qec::ErrorCategory::QECompilerError)
      .value("QECompilerNoInputError",
             qec::ErrorCategory::QECompilerNoInputError)
      .value("QECompilerCommunicationFailure",
             qec::ErrorCategory::QECompilerCommunicationFailure)
      .value("QECompilerEOFFailure",
             qec::ErrorCategory::QECompilerEOFFailure)
      .value("QECompilerNonZeroStatus",
             qec::ErrorCategory::QECompilerNonZeroStatus)
      .value("QECompilerSequenceTooLong",
             qec::ErrorCategory::QECompilerSequenceTooLong)
      .value("QECompilationFailure",
             qec::ErrorCategory::QECompilationFailure)
      .value("QELinkerNotImplemented",
             qec::ErrorCategory::QELinkerNotImplemented)
      .value("QELinkSignatureWarning",
             qec::ErrorCategory::QELinkSignatureWarning)
      .value("QELinkSignatureError",
             qec::ErrorCategory::QELinkSignatureError)
      .value("QELinkAddressError", qec::ErrorCategory::QELinkAddressError)
      .value("QELinkSignatureNotFound",
             qec::ErrorCategory::QELinkSignatureNotFound)
      .value("QELinkArgumentNotFoundWarning",
             qec::ErrorCategory::QELinkArgumentNotFoundWarning)
      .value("QELinkInvalidPatchTypeError",
             qec::ErrorCategory::QELinkInvalidPatchTypeError)
      .value("QEControlSystemResourcesExceeded",
             qec::ErrorCategory::QEControlSystemResourcesExceeded)
      .value("UncategorizedError", qec::ErrorCategory::UncategorizedError)
      .export_values();
}

void addSeverity(py::module &m) {
  py::enum_<qec::Severity>(m, "Severity")
      .value("Info", qec::Severity::Info)
      .value("Warning", qec::Severity::Warning)
      .value("Error", qec::Severity::Error)
      .value("Fatal", qec::Severity::Fatal)
      .export_values();
}

void addDiagnostic(py::module &m) {
  py::class_<qec::Diagnostic>(m, "Diagnostic")
      .def_readonly("severity", &qec::Diagnostic::severity)
      .def_readonly("category", &qec::Diagnostic::category)
      .def_readonly("message", &qec::Diagnostic::message)
      .def("__str__", &qec::Diagnostic::toString)
      .def(py::pickle(
          [](const qec::Diagnostic &d) {
            // __getstate__ serializes the C++ object into a tuple
            return py::make_tuple(d.severity, d.category, d.message);
          },
          [](py::tuple const &t) {
            // __setstate__ restores the C++ object from a tuple
            if (t.size() != 3)
              throw std::runtime_error("invalid state for unpickling");

            auto severity = t[0].cast<qec::Severity>();
            auto category = t[1].cast<qec::ErrorCategory>();
            auto message = t[2].cast<std::string>();

            return qec::Diagnostic(severity, category, std::move(message));
          }));
}
