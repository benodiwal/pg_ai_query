#pragma once
#include <string>

#include "spi_connection.hpp"

extern "C" {
#include <postgres.h>
#include <executor/spi.h>
}

namespace pg_ai {

enum class SPICommandType {
  OK_SELECT,
  OK_SELECT_INTO,
  OK_INSERT,
  OK_DELETE,
  OK_UPDATE,
  OK_MERGE,
  OK_INSERT_RETURNING,
  OK_DELETE_RETURNING,
  OK_UPDATE_RETURNING,
  OK_MERGE_RETURNING,
  OK_UTILITY,
  OK_REWRITTEN,
};

class SPICommandExecutor {
 public:
  SPICommandExecutor();
  ~SPICommandExecutor();

  SPICommandExecutor(const SPICommandExecutor&) = delete;
  SPICommandExecutor& operator=(const SPICommandExecutor&) = delete;
  SPICommandExecutor(SPICommandExecutor&&) = delete;
  SPICommandExecutor& operator=(SPICommandExecutor&&) = delete;

  std::pair<bool, std::string> execute(const char* command,
                                       bool isReadOnly,
                                       long n_rows,
                                       SPICommandType commandType,
                                       const std::string& queryName);
  SPIValue getCell(int row, int col) const;

 private:
  static int commandTypeToSpiMacro(SPICommandType command) {
    switch (command) {
      case SPICommandType::OK_SELECT:
        return SPI_OK_SELECT;
      case SPICommandType::OK_SELECT_INTO:
        return SPI_OK_SELINTO;
      case SPICommandType::OK_INSERT:
        return SPI_OK_INSERT;
      case SPICommandType::OK_DELETE:
        return SPI_OK_DELETE;
      case SPICommandType::OK_UPDATE:
        return SPI_OK_UPDATE;
      case SPICommandType::OK_MERGE:
        return SPI_OK_MERGE;
      case SPICommandType::OK_INSERT_RETURNING:
        return SPI_OK_INSERT_RETURNING;
      case SPICommandType::OK_DELETE_RETURNING:
        return SPI_OK_DELETE_RETURNING;
      case SPICommandType::OK_UPDATE_RETURNING:
        return SPI_OK_UPDATE_RETURNING;
      case SPICommandType::OK_UTILITY:
        return SPI_OK_UTILITY;
      case SPICommandType::OK_REWRITTEN:
        return SPI_OK_REWRITTEN;
      default:
        return SPI_OK_SELECT;
    }
  }

  SPIConnection
      spi_connection;  // this has to exist as a member variable, so that its
                       // destructor called on finishing of the executor
  SPITupleTable* tuptable;
};

}  // namespace pg_ai
