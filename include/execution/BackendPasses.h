#ifndef EXECUTION_BACKENDPASSES_H
#define EXECUTION_BACKENDPASSES_H
#include <memory>
#include <mlir/Pass/Pass.h>
namespace execution {
std::unique_ptr<mlir::Pass> createEnforceCABI();
std::unique_ptr<mlir::Pass> createAnnotateProfilingDataPass();
} // namespace execution
#endif //EXECUTION_BACKENDPASSES_H
