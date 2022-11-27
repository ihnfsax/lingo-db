#include <fstream>
#include <iostream>
#include <string>

#include "execution/Execution.h"
#include "execution/Timing.h"
#include "mlir-support/eval.h"

void check(bool b, std::string message) {
   if (!b) {
      std::cerr << "ERROR: " << message << std::endl;
      exit(1);
   }
}
int main(int argc, char** argv) {
   std::string inputFileName = std::string(argv[1]);

   runtime::ExecutionContext context;
   context.id = 42;
   if (argc <= 2) {
      std::cerr << "USAGE: run-sql *.sql database" << std::endl;
      return 1;
   }
   std::cout << "Loading Database from: " << argv[2] << '\n';
   auto database = runtime::Database::loadFromDir(std::string(argv[2]));
   context.db = std::move(database);

   support::eval::init();
   execution::ExecutionMode runMode = execution::getExecutionMode();
   auto queryExecutionConfig = execution::createQueryExecutionConfig(runMode, true);
   if (const char* numRuns = std::getenv("QUERY_RUNS")) {
      queryExecutionConfig->executionBackend->setNumRepetitions(std::atoi(numRuns));
      std::cout << "using " << queryExecutionConfig->executionBackend->getNumRepetitions() << " runs" << std::endl;
   }
   queryExecutionConfig->timingProcessor=std::make_unique<execution::TimingPrinter>(inputFileName);
   auto executer = execution::QueryExecuter::createDefaultExecuter(std::move(queryExecutionConfig));
   executer->fromFile(inputFileName);
   executer->setExecutionContext(&context);
   executer->execute();
   return 0;
}
