/* Copyright (c) 2012-2017 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>
#include <node_buffer.h>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "leveldown.h"
#include "database.h"
#include "batch.h"
#include "iterator.h"
#include "common.h"

namespace leveldown {

static Nan::Persistent<v8::FunctionTemplate> database_constructor;

Database::Database (const v8::Local<v8::Value>& from)
  : location(new Nan::Utf8String(from))
  , db(NULL)
  , currentIteratorId(0)
  , blockCache(NULL)
  , filterPolicy(NULL) {};

Database::~Database () {
  CloseDatabase();
  // if (db != NULL)
  //   delete db;
  delete location;
};

/* Calls from worker threads, NO V8 HERE *****************************/

leveldb::Status Database::OpenDatabase (
        leveldb::Options* options
    ) {
  return leveldb::DB::Open(*options, **location, &db);
}

leveldb::Status Database::PutToDatabase (
        leveldb::WriteOptions* options
      , leveldb::Slice key
      , leveldb::Slice value
    ) {
  return db->Put(*options, key, value);
}

leveldb::Status Database::GetFromDatabase (
        leveldb::ReadOptions* options
      , leveldb::Slice key
      , std::string& value
    ) {
  return db->Get(*options, key, &value);
}

leveldb::Status Database::DeleteFromDatabase (
        leveldb::WriteOptions* options
      , leveldb::Slice key
    ) {
  return db->Delete(*options, key);
}

leveldb::Status Database::WriteBatchToDatabase (
        leveldb::WriteOptions* options
      , leveldb::WriteBatch* batch
    ) {
  return db->Write(*options, batch);
}

uint64_t Database::ApproximateSizeFromDatabase (const leveldb::Range* range) {
  uint64_t size;
  db->GetApproximateSizes(range, 1, &size);
  //printf("ApproximateSize(%s, %s)=%lld\n", range->start.ToString().c_str(), range->limit.ToString().c_str(), size);
  return size;
}

void Database::CompactRangeFromDatabase (const leveldb::Slice* start,
                                         const leveldb::Slice* end) {
  db->CompactRange(start, end);
}

void Database::GetPropertyFromDatabase (
      const leveldb::Slice& property
    , std::string* value) {

  db->GetProperty(property, value);
}

leveldb::Iterator* Database::NewIterator (leveldb::ReadOptions* options) {
  return db->NewIterator(*options);
}

const leveldb::Snapshot* Database::NewSnapshot () {
  return db->GetSnapshot();
}

void Database::ReleaseSnapshot (const leveldb::Snapshot* snapshot) {
  return db->ReleaseSnapshot(snapshot);
}

void Database::ReleaseIterator (uint32_t id) {
  // called each time an Iterator is End()ed, in the main thread
  // we have to remove our reference to it and if it's the last iterator
  // we have to invoke a pending CloseWorker if there is one
  // if there is a pending CloseWorker it means that we're waiting for
  // iterators to end before we can close them
  iterators.erase(id);
}

void Database::CloseIterators () {
  if (!iterators.empty()) {
    std::map< uint32_t, leveldown::Iterator * >::iterator it = iterators.begin();
    while (it != iterators.end()) {
    // for (
    //   std::map< uint32_t, leveldown::Iterator * >::iterator it
    //       = iterators.begin()
    // ; it != iterators.end()
    // ; ++it) {

      // for each iterator still open, first check if it's already in
      // the process of ending (ended==true means an async End() is
      // in progress), if not, then we call End() with an empty callback
      // function and wait for it to hit ReleaseIterator() where our
      // CloseWorker will be invoked
      // printf("\nCloseIterators1: iterator:%d\n", it->first);

      leveldown::Iterator *iterator = it->second;
      ++it;
      // printf("\nCloseIterators: iterator:%d\n", iterator->id);
      //should close the iterator before closing database.
      //The close will iterators.erase(id)
      iterator->Close();
    }
  }
}

void Database::CloseDatabase () {
  CloseIterators();
  // printf("\nClosedIterators\n");

  delete db;
  db = NULL;
  // printf("\ndestroy dbIterator:%d\n", dbIterator);
  if (blockCache) {
    delete blockCache;
    blockCache = NULL;
  }
  if (filterPolicy) {
    delete filterPolicy;
    filterPolicy = NULL;
  }
}

/* V8 exposed functions *****************************/

NAN_METHOD(LevelDOWN) {
  v8::Local<v8::String> location = info[0].As<v8::String>();
  info.GetReturnValue().Set(Database::NewInstance(location));
}

void Database::Init () {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Database::New);
  database_constructor.Reset(tpl);
  tpl->SetClassName(Nan::New("Database").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  Nan::SetPrototypeMethod(tpl, "approximateSizeSync", Database::ApproximateSizeSync);
  Nan::SetPrototypeMethod(tpl, "getProperty", Database::GetProperty);
  Nan::SetPrototypeMethod(tpl, "iterator", Database::Iterator);
  Nan::SetPrototypeMethod(tpl, "openSync", Database::OpenSync);
  Nan::SetPrototypeMethod(tpl, "closeSync", Database::CloseSync);
  Nan::SetPrototypeMethod(tpl, "getSync", Database::GetSync);
  Nan::SetPrototypeMethod(tpl, "putSync", Database::PutSync);
  Nan::SetPrototypeMethod(tpl, "delSync", Database::DeleteSync);
  Nan::SetPrototypeMethod(tpl, "batchSync", Database::BatchSync);
  Nan::SetPrototypeMethod(tpl, "isExistsSync", Database::IsExistsSync);
  Nan::SetPrototypeMethod(tpl, "mGetSync", Database::MultiGetSync);
  Nan::SetPrototypeMethod(tpl, "getBufferSync", Database::GetBufferSync);
  Nan::SetPrototypeMethod(tpl, "compactRangeSync", Database::CompactRangeSync);
}

NAN_METHOD(Database::New) {
  Database* obj = new Database(info[0]);
  obj->Wrap(info.This());

  info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Value> Database::NewInstance (v8::Local<v8::String> &location) {
  Nan::EscapableHandleScope scope;

  Nan::MaybeLocal<v8::Object> maybeInstance;
  v8::Local<v8::Object> instance;

  v8::Local<v8::FunctionTemplate> constructorHandle =
      Nan::New<v8::FunctionTemplate>(database_constructor);

  v8::Local<v8::Value> argv[] = { location };
  maybeInstance = Nan::NewInstance(constructorHandle->GetFunction(), 1, argv);

  if (maybeInstance.IsEmpty())
      Nan::ThrowError("Could not create new Database instance");
  else
    instance = maybeInstance.ToLocalChecked();
  return scope.Escape(instance);
}

NAN_METHOD(Database::OpenSync) {
  leveldown::Database* database = Nan::ObjectWrap::Unwrap<leveldown::Database>(info.This());
  v8::Local<v8::Object> optionsObj;
  if (info.Length() > 0 && info[0]->IsObject()) {
    optionsObj = info[0].As<v8::Object>();
  }

  bool createIfMissing = BooleanOptionValue(optionsObj, "createIfMissing", true);
  bool errorIfExists = BooleanOptionValue(optionsObj, "errorIfExists");
  bool compression = BooleanOptionValue(optionsObj, "compression", true);
  uint32_t cacheSize = UInt32OptionValue(optionsObj, "cacheSize", 8 << 20);
  uint32_t writeBufferSize = UInt32OptionValue(
      optionsObj
    , "writeBufferSize"
    , 4 << 20
  );
  uint32_t blockSize = UInt32OptionValue(optionsObj, "blockSize", 4096);
  uint32_t maxOpenFiles = UInt32OptionValue(optionsObj, "maxOpenFiles", 1000);
  uint32_t blockRestartInterval = UInt32OptionValue(
      optionsObj
    , "blockRestartInterval"
    , 16
  );
  uint32_t maxFileSize = UInt32OptionValue(optionsObj, "maxFileSize", 2 << 20);

  database->blockCache = leveldb::NewLRUCache(cacheSize);
  database->filterPolicy = leveldb::NewBloomFilterPolicy(10);

  leveldb::Options options = leveldb::Options();
  options.block_cache            = database->blockCache;
  options.filter_policy          = database->filterPolicy;
  options.create_if_missing      = createIfMissing;
  options.error_if_exists        = errorIfExists;
  options.compression            = compression
      ? leveldb::kSnappyCompression
      : leveldb::kNoCompression;
  options.write_buffer_size      = writeBufferSize;
  options.block_size             = blockSize;
  options.max_open_files         = maxOpenFiles;
  options.block_restart_interval = blockRestartInterval;
  options.max_file_size          = maxFileSize;
  leveldb::Status status = database->OpenDatabase(&options);

  LD_METHOD_CHECK_DB_ERROR(openSync)

  info.GetReturnValue().Set(true);
}

NAN_METHOD(Database::CloseSync) {
  leveldown::Database* database = Nan::ObjectWrap::Unwrap<leveldown::Database>(info.This());

  database->CloseDatabase();
  info.GetReturnValue().Set(true);
}

//PutSync(key, value, {sync:false})
NAN_METHOD(Database::PutSync) {
  LD_METHOD_SETUP_SIMPLE(putSync, 1, 2)

  v8::Local<v8::Object> keyHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  v8::Local<v8::Object> valueHandle = Nan::To<v8::Object>(info[1]).ToLocalChecked();
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key);
  LD_STRING_OR_BUFFER_TO_SLICE(value, valueHandle, value);

  bool sync = BooleanOptionValue(optionsObj, "sync");

  leveldb::WriteOptions options = leveldb::WriteOptions();
  options.sync = sync;
  // leveldb::Status status = database->db->Put(options, *key, *value);
  leveldb::Status status = database->PutToDatabase(&options, key, value);
  DisposeStringOrBufferFromSlice(keyHandle, key);
  DisposeStringOrBufferFromSlice(valueHandle, value);

  LD_METHOD_CHECK_DB_ERROR(putSync)

  info.GetReturnValue().Set(true);
}

//getSync(aKey, {fillCache:true})
NAN_METHOD(Database::GetSync) {
  LD_METHOD_SETUP_SIMPLE(getSync, 0, 1)

  v8::Local<v8::Object> keyHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key);
  std::string value;

  // bool asBuffer = BooleanOptionValue(optionsObj, "asBuffer", true);
  bool fillCache = BooleanOptionValue(optionsObj, "fillCache", true);

  leveldb::ReadOptions options = leveldb::ReadOptions();
  options.fill_cache = fillCache;
  leveldb::Status status = database->GetFromDatabase(&options, key, value);
  DisposeStringOrBufferFromSlice(keyHandle, key);

  LD_METHOD_CHECK_DB_ERROR(getSync)

  v8::Local<v8::Value> returnValue = Nan::New<v8::String>((char*)value.data(), value.size()).ToLocalChecked();
  //printf("\ndb.get(%s)=%s\n", *key, *NanUtf8String(returnValue));
  info.GetReturnValue().Set(returnValue);
}

NAN_METHOD(Database::DeleteSync) {
  LD_METHOD_SETUP_SIMPLE(delSync, 1, 2)

  v8::Local<v8::Object> keyHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key);

  bool sync = BooleanOptionValue(optionsObj, "sync");

  leveldb::WriteOptions options = leveldb::WriteOptions();
  options.sync = sync;
  leveldb::Status status = database->DeleteFromDatabase(&options, key);
  DisposeStringOrBufferFromSlice(keyHandle, key);

  LD_METHOD_CHECK_DB_ERROR(delSync)

  info.GetReturnValue().Set(true);
}

//BatchSync(operations, {sync:true})
NAN_METHOD(Database::BatchSync) {
  if ((info.Length() == 0 || info.Length() == 1) && !info[0]->IsArray()) {
    v8::Local<v8::Object> optionsObj;
    if (info.Length() > 0 && info[0]->IsObject()) {
      optionsObj = Nan::To<v8::Object>(info[0]).ToLocalChecked();
    }
    info.GetReturnValue().Set(Batch::NewInstance(info.This(), optionsObj));
    return;
  }

  LD_METHOD_SETUP_SIMPLE(batchSync, 0, 1);

  bool sync = BooleanOptionValue(optionsObj, "sync");

  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(info[0]);

  leveldb::WriteBatch batch = leveldb::WriteBatch();

  bool hasData = false;

  for (unsigned int i = 0; i < array->Length(); i++) {
    if (!array->Get(i)->IsObject())
      continue;

    v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(array->Get(i));
    v8::Local<v8::Value> keyBuffer = obj->Get(Nan::New("key").ToLocalChecked());
    v8::Local<v8::Value> type = obj->Get(Nan::New("type").ToLocalChecked());

    if (type->StrictEquals(Nan::New("del").ToLocalChecked())) {
      LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)

      batch.Delete(key);
      if (!hasData)
        hasData = true;

      DisposeStringOrBufferFromSlice(keyBuffer, key);
    } else if (type->StrictEquals(Nan::New("put").ToLocalChecked())) {
      v8::Local<v8::Value> valueBuffer = obj->Get(Nan::New("value").ToLocalChecked());

      LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)
      LD_STRING_OR_BUFFER_TO_SLICE(value, valueBuffer, value)
      batch.Put(key, value);
      if (!hasData)
        hasData = true;

      DisposeStringOrBufferFromSlice(keyBuffer, key);
      DisposeStringOrBufferFromSlice(valueBuffer, value);
    }
  }

  if (hasData) {
    leveldb::WriteOptions options = leveldb::WriteOptions();

    options.sync = sync;
    leveldb::Status status = database->WriteBatchToDatabase(&options, &batch);
    LD_METHOD_CHECK_DB_ERROR(batchSync)
  }
  info.GetReturnValue().Set(hasData);
}

NAN_METHOD(Database::ApproximateSizeSync) {
  LD_METHOD_SETUP_SIMPLE(approximateSizeSync, 1, -1);

  v8::Local<v8::Object> startHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  v8::Local<v8::Object> endHandle = Nan::To<v8::Object>(info[1]).ToLocalChecked();

  LD_STRING_OR_BUFFER_TO_SLICE(start, startHandle, start)
  LD_STRING_OR_BUFFER_TO_SLICE(end, endHandle, end)

  leveldb::Range r(start, end);
  uint64_t size = database->ApproximateSizeFromDatabase(&r);
  DisposeStringOrBufferFromSlice(startHandle, start);
  DisposeStringOrBufferFromSlice(endHandle, end);
  v8::Local<v8::Value> returnValue = Nan::New<v8::Number>((double) size);

  info.GetReturnValue().Set(returnValue);
}

NAN_METHOD(Database::CompactRangeSync) {
  LD_METHOD_SETUP_SIMPLE(compactRangeSync, 1, -1);

  v8::Local<v8::Object> startHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  v8::Local<v8::Object> endHandle = Nan::To<v8::Object>(info[1]).ToLocalChecked();

  LD_STRING_OR_BUFFER_TO_SLICE(start, startHandle, start)
  LD_STRING_OR_BUFFER_TO_SLICE(end, endHandle, end)

  database->CompactRangeFromDatabase(&start, &end);

  DisposeStringOrBufferFromSlice(startHandle, start);
  DisposeStringOrBufferFromSlice(endHandle, end);

  info.GetReturnValue().Set(true);
}

NAN_METHOD(Database::GetProperty) {
  v8::Local<v8::Value> propertyHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  v8::Local<v8::Function> callback; // for LD_STRING_OR_BUFFER_TO_SLICE

  LD_STRING_OR_BUFFER_TO_SLICE(property, propertyHandle, property)

  leveldown::Database* database =
      Nan::ObjectWrap::Unwrap<leveldown::Database>(info.This());

  std::string* value = new std::string();
  database->GetPropertyFromDatabase(property, value);
  v8::Local<v8::String> returnValue
      = Nan::New<v8::String>(value->c_str(), value->length()).ToLocalChecked();
  delete value;
  delete[] property.data();

  info.GetReturnValue().Set(returnValue);
}

NAN_METHOD(Database::Iterator) {
  Database* database = Nan::ObjectWrap::Unwrap<Database>(info.This());

  v8::Local<v8::Object> optionsObj;
  if (info.Length() > 0 && info[0]->IsObject()) {
    optionsObj = v8::Local<v8::Object>::Cast(info[0]);
  }

  // each iterator gets a unique id for this Database, so we can
  // easily store & lookup on our `iterators` map
  uint32_t id = database->currentIteratorId++;
  Nan::TryCatch try_catch;
  v8::Local<v8::Object> iteratorHandle = Iterator::NewInstance(
      info.This()
    , Nan::New<v8::Number>(id)
    , optionsObj
  );
  if (try_catch.HasCaught()) {
    // NB: node::FatalException can segfault here if there is no room on stack.
    return Nan::ThrowError("Fatal Error in Database::Iterator!");
  }

  leveldown::Iterator *iterator =
      Nan::ObjectWrap::Unwrap<leveldown::Iterator>(iteratorHandle);

  database->iterators[id] = iterator;

  // register our iterator
  /*
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  obj->Set(Nan::New("iterator"), iteratorHandle);
  Nan::Persistent<v8::Object> persistent;
  persistent.Reset(nan_isolate, obj);
  database->iterators.insert(std::pair< uint32_t, Nan::Persistent<v8::Object> & >
      (id, persistent));
  */

  info.GetReturnValue().Set(iteratorHandle);
}

//mgetSync(keys, {fillCache:true, needKeyName:true, raiseError:true})
NAN_METHOD(Database::MultiGetSync) {
  LD_METHOD_SETUP_SIMPLE(mgetSync, 0, 1)

  v8::Local<v8::Value> v = info[0];
  if (v.IsEmpty()) {
    return info.GetReturnValue().SetUndefined();
  }
  if (!v->IsArray()) {
    return Nan::ThrowError(Nan::ErrnoException(kInvalidArgument,"mGetSync","mGetSync: the keys argument should be an array."));
  }

  bool fillCache = BooleanOptionValue(optionsObj, "fillCache", true);
  bool needKeyName = BooleanOptionValue(optionsObj, "keys", true);
  bool raiseError = BooleanOptionValue(optionsObj, "raiseError", true);

  v8::Local<v8::Array> keys = v8::Local<v8::Array>::Cast(v);

  leveldb::ReadOptions options = leveldb::ReadOptions();
  options.fill_cache = fillCache;

  size_t arraySize = keys->Length();
  v8::Local<v8::Array> returnArray = Nan::New<v8::Array>();
  int j = 0;
  for (unsigned int i = 0; i < arraySize; i++) {
    v = keys->Get(i);
    leveldb::Slice key = StringOrBufferToSlice(v);
    std::string value;
    leveldb::Status status = database->GetFromDatabase(&options, key, value);
    DisposeStringOrBufferFromSlice(v, key);

    if (status.ok()) {
      if (needKeyName) {
        returnArray->Set(Nan::New<v8::Integer>(j), v);
        ++j;
      }
      returnArray->Set(
        Nan::New<v8::Integer>(j),
        Nan::New<v8::String>((char*)value.data(), value.size()).ToLocalChecked()
      );
      ++j;
    } else if (raiseError) {
      Status* st = reinterpret_cast<Status*>(&status);
      return Nan::ThrowError(
        Nan::ErrnoException(st->code(), "mGetSync", status.ToString().c_str())
      );
    } else {
      if (needKeyName) {
        returnArray->Set(Nan::New<v8::Integer>(j), v);
        ++j;
      }
      returnArray->Set(Nan::New<v8::Integer>(j), Nan::Undefined());
      ++j;
    }
  }

  info.GetReturnValue().Set(returnArray);
}

//isExistsSync(key, {fillCache:true})
NAN_METHOD(Database::IsExistsSync) {
  LD_METHOD_SETUP_SIMPLE(getSync, 0, 1)

  bool result = false;
  v8::Local<v8::Object> keyHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key);
  std::string value;

  bool fillCache = BooleanOptionValue(optionsObj, "fillCache", true);

  leveldb::ReadOptions options = leveldb::ReadOptions();
  options.fill_cache = fillCache;
  leveldb::Status status = database->GetFromDatabase(&options, key, value);
  DisposeStringOrBufferFromSlice(keyHandle, key);

  if (status.ok()) {
    result = true;
  } else if (!status.IsNotFound()){
    Status* st = reinterpret_cast<Status*>(&status);
    return Nan::ThrowError(
      Nan::ErrnoException(st->code(), "isExistsSync", status.ToString().c_str())
    );
  }

  info.GetReturnValue().Set(result);
}

//getBufferSync(aKey, destBuffer, {fillCache:true, offset:0})
//return the value size
NAN_METHOD(Database::GetBufferSync) {
  LD_METHOD_SETUP_SIMPLE(getSync, 1, 2)


  v8::Local<v8::Object> keyHandle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key);
  std::string value;

  bool hasBuffer = !info[1]->IsNull();
  bool fillCache = BooleanOptionValue(optionsObj, "fillCache", true);
  size_t offset = static_cast<size_t>(UInt32OptionValue(optionsObj, "offset", 0));

  leveldb::ReadOptions options = leveldb::ReadOptions();
  options.fill_cache = fillCache;
  leveldb::Status status = database->GetFromDatabase(&options, key, value);
  DisposeStringOrBufferFromSlice(keyHandle, key);

  LD_METHOD_CHECK_DB_ERROR(getBufferSync)

  uint32_t value_len = value.size();
  if (hasBuffer) {
    v8::Local<v8::Object> destBuffer = info[1]->ToObject();
    if (node::Buffer::HasInstance(destBuffer)) {
      size_t dest_len = node::Buffer::Length(destBuffer);
      if (offset > dest_len) {
        return Nan::ThrowRangeError("offset out of range");
      }
      if (value_len > (dest_len-offset)) value_len = dest_len-offset;
      char* dest_data = static_cast<char*>(node::Buffer::Data(destBuffer));
      memcpy(dest_data+offset, value.data(), value_len);
    }
  }
  info.GetReturnValue().Set(value_len);
}

} // namespace leveldown
