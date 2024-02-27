# (C) Copyright IBM 2023.
#
# This code is part of Qiskit.
#
# This code is licensed under the Apache License, Version 2.0 with LLVM
# Exceptions. You may obtain a copy of this license in the LICENSE.txt
# file in the root directory of this source tree.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

get_property(qec_api_libs GLOBAL PROPERTY QEC_API_LIBS)

set(llvm_code_gen_libraries
    LLVMPowerPCCodeGen
    LLVMPowerPCInfo
    LLVMPowerPCAsmParser
    LLVMPowerPCDesc
    LLVMX86CodeGen
    LLVMX86Info
    LLVMX86AsmParser
    LLVMX86Desc
    LLVMX86TargetMCA
)

# Add AArch64 libraries if present in LLVM build.
if ("LLVMAArch64CodeGen" IN_LIST LLVM_AVAILABLE_LIBS)
    list(APPEND llvm_code_gen_libraries
        LLVMAArch64CodeGen
        LLVMAArch64Info
        LLVMAArch64AsmParser
        LLVMAArch64Desc
    )
    message(STATUS "Adding AArch64 libraries to llvm_code_gen_libraries")
endif ()

qec_add_plugin(QECTargetMock QEC_TARGET_PLUGIN
Conversion/QUIRToStandard/QUIRToStandard.cpp
MockTarget.cpp
MockUtils.cpp
Transforms/QubitLocalization.cpp

ADDITIONAL_HEADER_DIRS
${CMAKE_CURRENT_SOURCE_DIR}
${QEC_INCLUDE_DIR}/HAL

LINK_LIBS
${qec_api_libs}
QECHAL
MLIRExecutionEngine
MLIROptLib
MLIRLLVMDialect
MLIRLLVMToLLVMIRTranslation
MLIRFuncTransforms
${llvm_code_gen_libraries}
PLUGIN_REGISTRATION_HEADERS
${CMAKE_CURRENT_SOURCE_DIR}/Target.inc
)
