#pragma once

extern "C" {
#include <postgres.h>

#include <executor/spi.h>
}

#include <string>

namespace pg_ai {

/**
 * @brief RAII wrapper for PostgreSQL SPI connections
 *
 * Automatically connects to SPI on construction and disconnects on destruction.
 * This ensures SPI connections are properly cleaned up even when exceptions
 * occur.
 */
class SPIConnection {
 public:
  SPIConnection();
  ~SPIConnection();

  SPIConnection(const SPIConnection&) = delete;
  SPIConnection& operator=(const SPIConnection&) = delete;

  SPIConnection(SPIConnection&& other) noexcept;
  SPIConnection& operator=(SPIConnection&& other) noexcept;

  /**
   * @brief Check if the SPI connection is valid
   */
  bool isConnected() const { return connected_; }

  /**
   * @brief Get the error message if connection failed
   */
  const std::string& getErrorMessage() const { return error_message_; }

  /**
   * @brief Explicit conversion to bool for easy checking
   */
  explicit operator bool() const { return connected_; }

 private:
  bool connected_ = false;
  std::string error_message_;
};

/**
 * @brief Helper struct for managing SPI values that need to be freed
 *
 * Automatically frees the value with pfree on destruction.
 */
class SPIValue {
 public:
  explicit SPIValue(char* value) : value_(value) {}
  ~SPIValue() {
    if (value_) {
      pfree(value_);
    }
  }

  SPIValue(const SPIValue&) = delete;
  SPIValue& operator=(const SPIValue&) = delete;

  SPIValue(SPIValue&& other) noexcept : value_(other.value_) {
    other.value_ = nullptr;
  }

  SPIValue& operator=(SPIValue&& other) noexcept {
    if (this != &other) {
      if (value_) {
        pfree(value_);
      }
      value_ = other.value_;
      other.value_ = nullptr;
    }
    return *this;
  }

  /**
   * @brief Get the raw char* value
   */
  char* get() const { return value_; }

  /**
   * @brief Convert to string, returns empty string if null
   */
  std::string toString() const { return value_ ? std::string(value_) : ""; }

  /**
   * @brief Check if the value is not null
   */
  explicit operator bool() const { return value_ != nullptr; }

 private:
  char* value_;
};

}  // namespace pg_ai
