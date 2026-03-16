#include "../include/spi_command_executor.hpp"

namespace pg_ai {
SPICommandExecutor::SPICommandExecutor() : tuptable(nullptr) {}

SPICommandExecutor::~SPICommandExecutor() = default;

std::pair<bool, std::string> SPICommandExecutor::execute(
    const char* command,
    bool isReadOnly,
    long n_rows,
    SPICommandType commandType,
    const std::string& queryName) {
  std::pair<bool, std::string> result = {false, ""};

  if (!spi_connection) {
    result.second = spi_connection.getErrorMessage();
    return result;
  }

  int ret = SPI_execute(command, isReadOnly, n_rows);

  if (ret < 0 || ret != commandTypeToSpiMacro(commandType)) {
    result.second = "Failed to execute " + queryName +
                    " query. SPI result code: " + std::to_string(ret) + " (" +
                    std::string(SPI_result_code_string(ret)) + "). ";
    return result;
  }

  if (SPI_processed == 0) {
    result.second = "No output from" + queryName + " query";
    return result;
  }

  result.first = true;
  tuptable = SPI_tuptable;
  return result;
}

SPIValue SPICommandExecutor::getCell(const int row, const int col) const {
  TupleDesc tuple_desc = tuptable->tupdesc;
  HeapTuple tuple = tuptable->vals[row];
  return SPIValue(SPI_getvalue(tuple, tuple_desc, col));
}
}  // namespace pg_ai
