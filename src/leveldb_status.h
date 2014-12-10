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

#ifndef LD_LEVELDB_INCLUDE_STATUS_H_
#define LD_LEVELDB_INCLUDE_STATUS_H_

#include <string>

namespace leveldown {

//hacked to leveldb::Status to visit the private code field.
class Status {
 public:
  int code() const {
    return (state_ == NULL) ? kOk : static_cast<Code>(state_[4]);
  }
 private:
  // OK status has a NULL state_.  Otherwise, state_ is a new[] array
  // of the following form:
  //    state_[0..3] == length of message
  //    state_[4]    == code
  //    state_[5..]  == message
  const char* state_;

  enum Code {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kNotSupported = 3,
    kInvalidArgument = 4,
    kIOError = 5
  };
};


}  // namespace leveldown

#endif  // LD_LEVELDB_INCLUDE_STATUS_H_
