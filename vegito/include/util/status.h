/**
 * NOLINT(legal/copyright)
 *
 * The file vegito/include/util/status.h adapts the design from project apache
 * arrow and v6d:
 *
 *    https://github.com/apache/arrow/blob/master/cpp/src/arrow/status.h
 *    https://github.com/v6d-io/v6d/blob/main/src/common/util/status.h
 *
 * which are original referred from leveldb and has the following license:
 *
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A Status encapsulates the result of an operation.  It may indicate success,
// or it may indicate an error with an associated error message.
//
// Multiple threads can invoke const methods on a Status without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Status must use
// external synchronization.
 */

#ifndef VEGITO_INCLUDE_UTIL_STATUS_H_
#define VEGITO_INCLUDE_UTIL_STATUS_H_

#include <iosfwd>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "util/macros.h"

// raise a std::runtime_error (inherits std::exception), don't FATAL
#ifndef GART_CHECK_OK
#define GART_CHECK_OK(status)                                                 \
  do {                                                                        \
    auto _ret = (status);                                                     \
    if (!_ret.ok()) {                                                         \
      std::clog << "[error] Check failed: " << _ret.ToString() << " in \""    \
                << #status << "\""                                            \
                << ", in function " << __PRETTY_FUNCTION__ << ", file "       \
                << __FILE__ << ", line " << GART_TO_STRING(__LINE__)          \
                << std::endl;                                                 \
      throw std::runtime_error("Check failed: " + _ret.ToString() +           \
                               " in \"" #status "\", in function " +          \
                               std::string(__PRETTY_FUNCTION__) + ", file " + \
                               __FILE__ + ", line " +                         \
                               GART_TO_STRING(__LINE__));                     \
    }                                                                         \
  } while (0)
#endif  // GART_CHECK_OK

// check the condition, raise and runtime error, rather than `FATAL` when false
#ifndef GART_ASSERT_NO_VERBOSE
#define GART_ASSERT_NO_VERBOSE(condition)                                 \
  do {                                                                    \
    if (!(condition)) {                                                   \
      std::clog << "[error] Assertion failed in \"" #condition "\""       \
                << ", in function '" << __PRETTY_FUNCTION__ << "', file " \
                << __FILE__ << ", line " << GART_TO_STRING(__LINE__)      \
                << std::endl;                                             \
      throw std::runtime_error(                                           \
          "Assertion failed in \"" #condition "\", in function '" +       \
          std::string(__PRETTY_FUNCTION__) + "', file " + __FILE__ +      \
          ", line " + GART_TO_STRING(__LINE__));                          \
    }                                                                     \
  } while (0)
#endif  // GART_ASSERT_NO_VERBOSE

// check the condition, raise and runtime error, rather than `FATAL` when false
#ifndef GART_ASSERT_VERBOSE
#define GART_ASSERT_VERBOSE(condition, message)                               \
  do {                                                                        \
    if (!(condition)) {                                                       \
      std::clog << "[error] Assertion failed in \"" #condition "\": "         \
                << std::string(message) << ", in function '"                  \
                << __PRETTY_FUNCTION__ << "', file " << __FILE__ << ", line " \
                << GART_TO_STRING(__LINE__) << std::endl;                     \
      throw std::runtime_error(                                               \
          "Assertion failed in \"" #condition "\": " + std::string(message) + \
          ", in function '" + std::string(__PRETTY_FUNCTION__) + "', file " + \
          __FILE__ + ", line " + GART_TO_STRING(__LINE__));                   \
    }                                                                         \
  } while (0)
#endif  // GART_ASSERT_VERBOSE

#ifndef GART_ASSERT
#define GART_ASSERT(...)                                              \
  GET_MACRO(__VA_ARGS__, GART_ASSERT_VERBOSE, GART_ASSERT_NO_VERBOSE) \
  (__VA_ARGS__)
#endif  // GART_ASSERT

// return the status if the status is not OK.
#ifndef GART_RETURN_ON_ERROR
#define GART_RETURN_ON_ERROR(status) \
  do {                               \
    auto _ret = (status);            \
    if (!_ret.ok()) {                \
      return _ret;                   \
    }                                \
  } while (0)
#endif  // GART_RETURN_ON_ERROR

// return a null pointer if the status is not OK.
#ifndef GART_RETURN_NULL_ON_ERROR
#define GART_RETURN_NULL_ON_ERROR(status)                                  \
  do {                                                                     \
    auto _ret = (status);                                                  \
    if (!_ret.ok()) {                                                      \
      std::clog << "[error] Check failed: " << _ret.ToString() << " in \"" \
                << #status << "\"" << std::endl;                           \
      return nullptr;                                                      \
    }                                                                      \
  } while (0)
#endif  // GART_RETURN_NULL_ON_ERROR

// return the status if the status is not OK.
#ifndef GART_RETURN_ON_ASSERT_NO_VERBOSE
#define GART_RETURN_ON_ASSERT_NO_VERBOSE(condition)       \
  do {                                                    \
    if (!(condition)) {                                   \
      return ::gart::Status::AssertionFailed(#condition); \
    }                                                     \
  } while (0)
#endif  // GART_RETURN_ON_ASSERT_NO_VERBOSE

// return the status if the status is not OK.
#ifndef GART_RETURN_ON_ASSERT_VERBOSE
#define GART_RETURN_ON_ASSERT_VERBOSE(condition, message)                   \
  do {                                                                      \
    if (!(condition)) {                                                     \
      return ::gart::Status::AssertionFailed(std::string(#condition ": ") + \
                                             message);                      \
    }                                                                       \
  } while (0)
#endif  // GART_RETURN_ON_ASSERT_VERBOSE

#ifndef GART_RETURN_ON_ASSERT
#define GART_RETURN_ON_ASSERT(...)                 \
  GET_MACRO(__VA_ARGS__, RETURN_ON_ASSERT_VERBOSE, \
            RETURN_ON_ASSERT_NO_VERBOSE)           \
  (__VA_ARGS__)
#endif  // GART_RETURN_ON_ASSERT

// return a null pointer if the status is not OK.
#ifndef GART_RETURN_NULL_ON_ASSERT_NO_VERBOSE
#define GART_RETURN_NULL_ON_ASSERT_NO_VERBOSE(condition)                  \
  do {                                                                    \
    if (!(condition)) {                                                   \
      std::clog << "[error] Assertion failed in \"" #condition "\""       \
                << ", in function '" << __PRETTY_FUNCTION__ << "', file " \
                << __FILE__ << ", line " << GART_TO_STRING(__LINE__)      \
                << std::endl;                                             \
    }                                                                     \
  } while (0)
#endif  // GART_RETURN_NULL_ON_ASSERT_NO_VERBOSE

// return a null pointer if the status is not OK.
#ifndef GART_RETURN_NULL_ON_ASSERT_VERBOSE
#define GART_RETURN_NULL_ON_ASSERT_VERBOSE(condition, message)                \
  do {                                                                        \
    if (!(condition)) {                                                       \
      std::clog << "[error] Assertion failed in \"" #condition "\": "         \
                << std::string(message) << ", in function '"                  \
                << __PRETTY_FUNCTION__ << "', file " << __FILE__ << ", line " \
                << GART_TO_STRING(__LINE__) << std::endl;                     \
    }                                                                         \
  } while (0)
#endif  // GART_RETURN_NULL_ON_ASSERT_VERBOSE

// return a null pointer if the status is not OK.
#ifndef GART_RETURN_NULL_ON_ASSERT
#define GART_RETURN_NULL_ON_ASSERT(...)                 \
  GET_MACRO(__VA_ARGS__, RETURN_NULL_ON_ASSERT_VERBOSE, \
            RETURN_NULL_ON_ASSERT_NO_VERBOSE)           \
  (__VA_ARGS__)
#endif  // GART_RETURN_NULL_ON_ASSERT

// discard and ignore the error status.
#ifndef GART_DISCARD
#define GART_DISCARD(status)                                  \
  do {                                                        \
    auto _ret = (status);                                     \
    if (!_ret.ok()) {} /* NOLINT(whitespace/empty_if_body) */ \
  } while (0)
#endif  // GART_DISCARD

// suppress and ignore the error status, deprecated in favour of
// GART_DISCARD
#ifndef GART_SUPPRESS
#define GART_SUPPRESS(status)                                 \
  do {                                                        \
    auto _ret = (status);                                     \
    if (!_ret.ok()) {} /* NOLINT(whitespace/empty_if_body) */ \
  } while (0)
#endif  // GART_SUPPRESS

// print the error message when failed, but never throw or abort.
#ifndef GART_LOG_ERROR
#define GART_LOG_ERROR(status)                                             \
  do {                                                                     \
    auto _ret = (status);                                                  \
    if (!_ret.ok()) {                                                      \
      std::clog << "[error] Check failed: " << _ret.ToString() << " in \"" \
                << #status << "\"" << std::endl;                           \
    }                                                                      \
  } while (0)
#endif  // GART_LOG_ERROR

namespace gart {

enum class StatusCode : unsigned char {
  kOK = 0,
  kKeyError = 1,
  kTypeError = 2,
  kInvalid = 3,
  kIndexError = 4,
  kOutOfMemory = 5,
  kKafkaConnectError = 6,
  kCapturerError = 7,
  kParseJsonError = 8,
  kOpenFileError = 9,
  kParseLogError = 10,
  kOperationError = 11,
  kGraphSchemaConfigError = 12,
  kTableConfigError = 13,
  kPremisionError = 14,
  kAssertionFailed = 15,

  kUnknownError = 255
};

/**
 * @brief A Status encapsulates the result of an operation.  It may indicate
 * success, or it may indicate an error with an associated error message.
 *
 * GART also provides macros for convenient error handling:
 *
 * - `GART_CHECK_OK`: used for check Gart's status, when error occurs,
 *   raise a runtime exception.
 *
 *   It should be used where the function itself doesn't return a `Status`, but
 *   we want to check the status of certain statements.
 *
 * - `GART_ASSERT`: used for assert on a condition, if false, raise a
 * runtime exception.
 *
 *   It should be used where the function itself doesn't return a `Status`, but
 *   we need to check on a condition.
 *
 * - `RETURN_ON_ERROR`: it looks like GART_CHECK_OK, but return the Status
 * if it is not ok, without causing an exception.
 *
 * - `RETURN_ON_ASSERT`: it looks like GART_ASSERT, but return a status of
 *   AssertionFailed, without causing an exception and abort the program.
 *
 * - `GART_DISCARD`: suppress and ignore the error status, just logging it
 *   out.
 *
 * - `GART_SUPPRESS`: suppress and ignore the error status, just logging it
 *   out, deprecated in favour of GART_DISCARD.
 *
 *   This one is usaully used in dtor that we shouldn't raise exceptions.
 */
class GART_MUST_USE_TYPE Status {
 public:
  Status() noexcept : state_(nullptr) {}
  ~Status() noexcept {
    if (state_ != nullptr) {
      DeleteState();
    }
  }
  Status(StatusCode code, const std::string& msg) {
    state_ = new State;
    state_->code = code;
    state_->msg = msg;
  }
  // Copy the specified status.
  inline Status(const Status& s);
  inline Status& operator=(const Status& s);

  // Move the specified status.
  inline Status(Status&& s) noexcept;
  inline Status& operator=(Status&& s) noexcept;

  // AND the statuses.
  inline Status operator&(const Status& s) const noexcept;
  inline Status operator&(Status&& s) const noexcept;
  inline Status& operator&=(const Status& s) noexcept;
  inline Status& operator&=(Status&& s) noexcept;
  inline Status& operator+=(const Status& s) noexcept;

  /// Return a success status
  inline static Status OK() { return Status(); }

  /// Wrap a status with customized extra message
  inline static Status Wrap(const Status& s, const std::string& message) {
    if (s.ok()) {
      return s;
    }
    return Status(s.code(), message + ": " + s.message());
  }

  /// Return an error status for invalid data, with user specified error
  /// message.
  static Status Invalid(std::string const& message = "") {
    return Status(StatusCode::kInvalid, message);
  }

  /// Return an error status for failed key lookups (e.g. column name in a
  /// table).
  static Status KeyError() { return Status(StatusCode::kKeyError, ""); }
  static Status KeyError(std::string const& msg) {
    return Status(StatusCode::kKeyError, msg);
  }

  /// Return an error status for type errors (such as mismatching data types).
  static Status TypeError(std::string const& msg = "") {
    return Status(StatusCode::kTypeError, msg);
  }

  /// Return an error status when an index is out of bounds.
  static Status IndexError(const std::string& msg = "") {
    return Status(StatusCode::kIndexError, msg);
  }

  /// Return an error status when property storage is full
  static Status OutOfMemory(std::string const& message = "") {
    return Status(StatusCode::kOutOfMemory, message);
  }

  /// Return a status code indicates kafka connection error.
  static Status KafkaConnectError(std::string const& message = "") {
    return Status(StatusCode::kKafkaConnectError, message);
  }

  /// Return an error when log capturer related error occurs.
  static Status CapturerError(std::string const& message = "") {
    return Status(StatusCode::kCapturerError, message);
  }

  /// error status for parse json error
  static Status ParseJsonError(std::string const& message = "") {
    return Status(StatusCode::kParseJsonError, message);
  }

  /// Return an error if fail open a file.
  static Status OpenFileError(std::string const& message = "") {
    return Status(StatusCode::kOpenFileError, "");
  }

  /// Return an error if parse log fails
  static Status ParseLogError(std::string const& message = "") {
    return Status(StatusCode::kParseLogError, message);
  }

  /// Return an error if unknwon operation
  static Status OperationError(std::string const& msg = "") {
    return Status(StatusCode::kOperationError, msg);
  }

  /// Return an error if graph schema is invalid.
  static Status GraphSchemaConfigError(std::string const& msg = "") {
    return Status(StatusCode::kGraphSchemaConfigError, msg);
  }

  /// Return an error if table schema is invalid.
  static Status TableConfigError(std::string const& msg = "") {
    return Status(StatusCode::kTableConfigError, msg);
  }

  /// Return an error if can not get required premision for RDMS.
  static Status PremisionError(std::string const& message = "") {
    return Status(StatusCode::kPremisionError, message);
  }

  /// Return an error status when the condition assertion is false.
  static Status AssertionFailed(std::string const& condition) {
    return Status(StatusCode::kAssertionFailed, condition);
  }

  /// Return an error status for unknown errors
  static Status UnknownError(std::string const& message = "") {
    return Status(StatusCode::kUnknownError, message);
  }

  /// Return true iff the status indicates success.
  bool ok() const { return (state_ == nullptr); }

  bool IsInvalid() const { return code() == StatusCode::kInvalid; }

  bool IsKeyError() const { return code() == StatusCode::kKeyError; }

  bool IsTypeError() const { return code() == StatusCode::kTypeError; }

  bool IsIndexError() const { return code() == StatusCode::kIndexError; }

  bool IsOutOfMemory() const { return code() == StatusCode::kOutOfMemory; }

  bool IsKafkaConnectError() const {
    return code() == StatusCode::kKafkaConnectError;
  }

  bool IsCapturerError() const { return code() == StatusCode::kCapturerError; }

  bool IsParseJsonError() const {
    return code() == StatusCode::kParseJsonError;
  }

  bool IsOpenFileError() const { return code() == StatusCode::kOpenFileError; }

  bool IsParseLogError() const { return code() == StatusCode::kParseLogError; }

  bool IsOperationError() const {
    return code() == StatusCode::kOperationError;
  }

  bool IsGraphSchemaConfigError() const {
    return code() == StatusCode::kGraphSchemaConfigError;
  }

  bool IsTableConfigError() const {
    return code() == StatusCode::kTableConfigError;
  }

  bool IsPremisionError() const {
    return code() == StatusCode::kPremisionError;
  }

  bool IsAssertionFailed() const {
    return code() == StatusCode::kAssertionFailed;
  }

  /// Return true iff the status indicates an unknown error.
  bool IsUnknownError() const { return code() == StatusCode::kUnknownError; }

  std::string ToString() const {
    std::string result(CodeAsString());
    if (state_ == nullptr) {
      return result;
    }
    result += ": ";
    result += state_->msg;
    return result;
  }

  std::string CodeAsString() const {
    if (state_ == nullptr) {
      return "OK";
    }

    const char* type;
    switch (code()) {
    case StatusCode::kOK:
      type = "OK";
      break;
    case StatusCode::kInvalid:
      type = "Invalid";
      break;
    case StatusCode::kKeyError:
      type = "Key error";
      break;
    case StatusCode::kTypeError:
      type = "Type error";
      break;
    case StatusCode::kIndexError:
      type = "Index error";
      break;
    case StatusCode::kOutOfMemory:
      type = "Out of Memory";
      break;
    case StatusCode::kKafkaConnectError:
      type = "Kafka connect error";
      break;
    case StatusCode::kCapturerError:
      type = "Capturer error";
      break;
    case StatusCode::kParseJsonError:
      type = "Parse json error";
      break;
    case StatusCode::kOpenFileError:
      type = "Open file error";
      break;
    case StatusCode::kParseLogError:
      type = "Parse log error";
      break;
    case StatusCode::kOperationError:
      type = "Operation error";
      break;
    case StatusCode::kGraphSchemaConfigError:
      type = "Graph schema config error";
      break;
    case StatusCode::kTableConfigError:
      type = "Table config error";
      break;
    case StatusCode::kAssertionFailed:
      type = "Assertion failed";
      break;
    case StatusCode::kUnknownError:
    default:
      type = "Unknown error";
      break;
    }
    return std::string(type);
  }

  /// \brief Return the StatusCode value attached to this status.
  StatusCode code() const { return ok() ? StatusCode::kOK : state_->code; }

  /// \brief Return the specific error message attached to this status.
  std::string message() const { return ok() ? "" : state_->msg; }

  template <typename T>
  Status& operator<<(const T& s);
  const std::string Backtrace() const { return backtrace_; }

 private:
  struct State {
    StatusCode code;
    std::string msg;
  };
  // OK status has a `NULL` state_.  Otherwise, `state_` points to
  // a `State` structure containing the error code and message(s)
  State* state_;
  // OK status has a `NULL` backtrace_.
  std::string backtrace_;

  void DeleteState() {
    delete state_;
    state_ = nullptr;
  }
  void CopyFrom(const Status& s) {
    delete state_;
    if (s.state_ == nullptr) {
      state_ = nullptr;
    } else {
      state_ = new State(*s.state_);
    }
  }
  void MoveFrom(Status& s) {
    delete state_;
    state_ = s.state_;
    s.state_ = nullptr;
  }
  void MergeFrom(const Status& s) {
    delete state_;
    if (state_ == nullptr) {
      if (s.state_ != nullptr) {
        state_ = new State(*s.state_);
      }
    } else {
      if (s.state_ != nullptr) {
        state_->msg += "; " + s.state_->msg;
      }
    }
  }
};

inline std::ostream& operator<<(std::ostream& os, const Status& x) {
  os << x.ToString();
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const StatusCode& x) {
  os << (unsigned char) x;
  return os;
}

inline std::istream& operator>>(std::istream& is, StatusCode& x) {
  unsigned char c;
  is >> c;
  x = static_cast<StatusCode>(c);
  return is;
}

template <typename T>
Status& Status::operator<<(const T& s) {
  // CHECK_NE(state_, nullptr);  // Not allowed to append message to a OK
  // message;
  std::ostringstream tmp;
  tmp << s;
  state_->msg.append(tmp.str());
  return *this;
}

Status::Status(const Status& s)
    : state_((s.state_ == nullptr) ? nullptr : new State(*s.state_)) {}

Status& Status::operator=(const Status& s) {
  // The following condition catches both aliasing (when this == &s),
  // and the common case where both s and *this are ok.
  if (state_ != s.state_) {
    CopyFrom(s);
  }
  return *this;
}

Status::Status(Status&& s) noexcept : state_(s.state_) { s.state_ = nullptr; }

Status& Status::operator=(Status&& s) noexcept {
  MoveFrom(s);
  return *this;
}

/// \cond FALSE
// (note: emits warnings on Doxygen < 1.8.15,
//  see https://github.com/doxygen/doxygen/issues/6295)
Status Status::operator&(const Status& s) const noexcept {
  if (ok()) {
    return s;
  } else {
    return *this;
  }
}

Status Status::operator&(Status&& s) const noexcept {
  if (ok()) {
    return std::move(s);
  } else {
    return *this;
  }
}

Status& Status::operator&=(const Status& s) noexcept {
  if (ok() && !s.ok()) {
    CopyFrom(s);
  }
  return *this;
}

Status& Status::operator&=(Status&& s) noexcept {
  if (ok() && !s.ok()) {
    MoveFrom(s);
  }
  return *this;
}

Status& Status::operator+=(const Status& s) noexcept {
  if (!s.ok()) {
    MergeFrom(s);
  }
  return *this;
}

/// \endcond

}  // namespace gart

#endif  // VEGITO_INCLUDE_UTIL_STATUS_H_
