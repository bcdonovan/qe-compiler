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

package_add_test_with_libs(unittest-quir-dialect
        quir-dialect.cpp

        LIBRARIES
        QECLib
)

set(TEST_FILES
        Payload/PayloadRegistryTest.cpp
        )

if (QEC_WITH_MOCK_TARGET)
    set(TEST_FILES
            HAL/TargetSystemRegistryTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}
            )
endif ()

package_add_test_with_libs(unittest-qe-compiler
        ${TEST_FILES}

        LIBRARIES
        QECLib
        )

add_custom_target(run-qe-compiler-unittests
        COMMAND ${CMAKE_CTEST_COMMAND}
        DEPENDS ${QEC_UNITTESTS}
        )
