//===- SchedulePortModule.h  - Schedule Pulse on single port ----*- C++ -*-===//
//
// (C) Copyright IBM 2022, 2023.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.
//
//===----------------------------------------------------------------------===//
///
///  This file implements the pass for scheduling on a single port. The
///  pass operates at the module level. For an alternate pass which operates
///  at the sequence level see: SchedulePortSequence.{h,cpp}. Functionality
///  common to both passes is implemented in Utils/SchedulePort.{h,cpp}
///
///  A single port may have multiple frames mixed with it (measurement vs drive,
///  etc). Each mixed frame will have delay and play operations on the mixed
///  frame which need to be processed down to a set of delays and plays
///  on the underlying port.
///
///  See SchedulePort.cpp for more detailed background.
//===----------------------------------------------------------------------===//

#ifndef PULSE_SCHEDULE_PORT_MODULE_H
#define PULSE_SCHEDULE_PORT_MODULE_H

#include "Dialect/Pulse/IR/PulseOps.h"
#include "Utils/DebugIndent.h"
#include "mlir/Pass/Pass.h"

#include <deque>
#include <set>

namespace mlir::pulse {

class SchedulePortModulePass
    : public PassWrapper<SchedulePortModulePass, OperationPass<ModuleOp>>,
      protected qssc::utils::DebugIndent {
public:
  void runOnOperation() override;

  llvm::StringRef getArgument() const override;
  llvm::StringRef getDescription() const override;

private:
  uint processCall(Operation *module, CallSequenceOp &callSequenceOp);
};
} // namespace mlir::pulse

#endif // PULSE_SCHEDULE_CHANNEL_H