#include "../include/spi_connection.hpp"

namespace pg_ai {

SPIConnection::SPIConnection() {
  if (SPI_connect() == SPI_OK_CONNECT) {
    connected_ = true;
  } else {
    connected_ = false;
    error_message_ = "Failed to connect to SPI";
  }
}

SPIConnection::~SPIConnection() {
  if (connected_) {
    SPI_finish();
  }
}

SPIConnection::SPIConnection(SPIConnection&& other) noexcept
    : connected_(other.connected_),
      error_message_(std::move(other.error_message_)) {
  other.connected_ = false;
}

SPIConnection& SPIConnection::operator=(SPIConnection&& other) noexcept {
  if (this != &other) {
    if (connected_) {
      SPI_finish();
    }
    connected_ = other.connected_;
    error_message_ = std::move(other.error_message_);
    other.connected_ = false;
  }
  return *this;
}

}  // namespace pg_ai
