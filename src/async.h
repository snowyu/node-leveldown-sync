/* Copyright (c) 2012-2014 LevelDOWN contributors
 * See list at <https://github.com/rvagg/node-leveldown#contributing>
 * MIT License <https://github.com/rvagg/node-leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_ASYNC_H
#define LD_ASYNC_H

#include <node.h>
#include <nan.h>
#include "database.h"

namespace leveldown {

class Database;

/* abstract */ class AsyncWorker : public NanAsyncWorker {
public:
  AsyncWorker (
      leveldown::Database* database
    , NanCallback *callback
  ) : NanAsyncWorker(callback), database(database) {
    NanScope();
    v8::Local<v8::Object> obj = NanNew<v8::Object>();
    NanAssignPersistent(persistentHandle, obj);
  }

protected:
  void HandleErrorCallback() {
    NanScope();

    v8::Local<v8::Value> err = v8::Exception::Error(NanNew<v8::String>(ErrorMessage()));
    if (hasErrorCode) {
      v8::Local<v8::Object> obj = err.As<v8::Object>();
      obj->Set(NanNew<v8::String>("code"), NanNew<v8::Integer>(errorCode));
    }

    v8::Local<v8::Value> argv[] = {err};
    callback->Call(1, argv);
  }

  void SetErrorCode(const char *msg, const int code) {
    SetErrorMessage(msg);
    hasErrorCode = true;
    errorCode=code;
  }

  void SetStatus(leveldb::Status status) {
    this->status = status;
    if (!status.ok()) {
      Status* st = reinterpret_cast<Status*>(&status);
      SetErrorCode(status.ToString().c_str(), st->code());
    }
  }
  Database* database;
private:
  leveldb::Status status;
  bool hasErrorCode;
  int errorCode;
};

} // namespace leveldown

#endif
