/* Copyright (c) 2012-2017 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>

#include "leveldb_status.h"
#include "leveldown.h"
#include "database.h"
#include "iterator.h"
#include "batch.h"
#include "leveldown_async.h"

namespace leveldown {

NAN_METHOD(DestroyDBSync) {
  Nan::HandleScope scope;

  if (info.Length() == 0 || !info[0]->IsString())
    return Nan::ThrowError(Nan::ErrnoException(kInvalidArgument, "destroySync", "destroySync require location:string argument"));

  Nan::Utf8String* location = new Nan::Utf8String(info[0]);

  leveldb::Options options;
  leveldb::Status status = leveldb::DestroyDB(**location, options);
  delete location;
  LD_METHOD_CHECK_DB_ERROR(destroySync)

  info.GetReturnValue().Set(true);
}

NAN_METHOD(RepairDBSync) {
  Nan::HandleScope scope;

  if (info.Length() == 0 || !info[0]->IsString())
    return Nan::ThrowError(Nan::ErrnoException(kInvalidArgument, "repairSync", "repairSync require location:string argument"));

  Nan::Utf8String* location = new Nan::Utf8String(info[0]);

  leveldb::Options options;
  leveldb::Status status = leveldb::RepairDB(**location, options);
  delete location;
  LD_METHOD_CHECK_DB_ERROR(repairSync)

  info.GetReturnValue().Set(true);
}

NAN_METHOD(DestroyDB) {
  Nan::HandleScope scope;

  Nan::Utf8String* location = new Nan::Utf8String(info[0]);

  Nan::Callback* callback = new Nan::Callback(
      v8::Local<v8::Function>::Cast(info[1]));

  DestroyWorker* worker = new DestroyWorker(
      location
    , callback
  );

  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(RepairDB) {
  Nan::HandleScope scope;

  Nan::Utf8String* location = new Nan::Utf8String(info[0]);

  Nan::Callback* callback = new Nan::Callback(
      v8::Local<v8::Function>::Cast(info[1]));

  RepairWorker* worker = new RepairWorker(
      location
    , callback
  );

  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().SetUndefined();
}

void Init (v8::Local<v8::Object> target) {
  Database::Init();
  leveldown::Iterator::Init();
  leveldown::Batch::Init();

  v8::Local<v8::Function> leveldown =
      Nan::New<v8::FunctionTemplate>(LevelDOWN)->GetFunction();

  leveldown->Set(
      Nan::New("destroy").ToLocalChecked()
    , Nan::New<v8::FunctionTemplate>(DestroyDB)->GetFunction()
  );

  leveldown->Set(
      Nan::New("repair").ToLocalChecked()
    , Nan::New<v8::FunctionTemplate>(RepairDB)->GetFunction()
  );

  leveldown->Set(
      Nan::New("destroySync").ToLocalChecked()
    , Nan::New<v8::FunctionTemplate>(DestroyDBSync)->GetFunction()
  );

  leveldown->Set(
      Nan::New("repairSync").ToLocalChecked()
    , Nan::New<v8::FunctionTemplate>(RepairDBSync)->GetFunction()
  );

  target->Set(Nan::New("leveldown").ToLocalChecked(), leveldown);
}

NODE_MODULE(leveldown, Init)

} // namespace leveldown
