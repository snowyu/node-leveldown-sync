/* Copyright (c) 2012-2017 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_DATABASE_H
#define LD_DATABASE_H

#include <map>
#include <vector>
#include <node.h>

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <nan.h>

#include "leveldb_status.h"
#include "leveldown.h"
#include "iterator.h"

namespace leveldown {

const int kOk = 0;
const int kNotFound = 1;
const int kCorruption = 2;
const int kNotSupported = 3;
const int kInvalidArgument = 4;
const int kIOError = 5;
const int kNotOpened = 6;

NAN_METHOD(LevelDOWN);

struct Reference {
  Nan::Persistent<v8::Object> handle;
  leveldb::Slice slice;

  Reference(v8::Local<v8::Value> obj, leveldb::Slice slice) : slice(slice) {
    v8::Local<v8::Object> _obj = Nan::New<v8::Object>();
    _obj->Set(Nan::New("obj").ToLocalChecked(), obj);
    handle.Reset(_obj);
  };
};

static inline void ClearReferences (std::vector<Reference *> *references) {
  for (std::vector<Reference *>::iterator it = references->begin()
      ; it != references->end()
      ; ) {
    DisposeStringOrBufferFromSlice((*it)->handle, (*it)->slice);
    it = references->erase(it);
  }
  delete references;
}

class Database : public Nan::ObjectWrap {
public:
  static void Init ();
  static v8::Local<v8::Value> NewInstance (v8::Local<v8::String> &location);

  leveldb::Status OpenDatabase (leveldb::Options* options);
  leveldb::Status PutToDatabase (
      leveldb::WriteOptions* options
    , leveldb::Slice key
    , leveldb::Slice value
  );
  leveldb::Status GetFromDatabase (
      leveldb::ReadOptions* options
    , leveldb::Slice key
    , std::string& value
  );
  leveldb::Status DeleteFromDatabase (
      leveldb::WriteOptions* options
    , leveldb::Slice key
  );
  leveldb::Status WriteBatchToDatabase (
      leveldb::WriteOptions* options
    , leveldb::WriteBatch* batch
  );
  uint64_t ApproximateSizeFromDatabase (const leveldb::Range* range);
  void CompactRangeFromDatabase (const leveldb::Slice* start, const leveldb::Slice* end);
  void GetPropertyFromDatabase (const leveldb::Slice& property, std::string* value);
  leveldb::Iterator* NewIterator (leveldb::ReadOptions* options);
  const leveldb::Snapshot* NewSnapshot ();
  void ReleaseSnapshot (const leveldb::Snapshot* snapshot);
  void CloseIterators ();
  void CloseDatabase ();
  void ReleaseIterator (uint32_t id);

  Database (const v8::Local<v8::Value>& from);
  ~Database ();

private:
  Nan::Utf8String* location;
  leveldb::DB* db;
  uint32_t currentIteratorId;
  leveldb::Cache* blockCache;
  const leveldb::FilterPolicy* filterPolicy;

  std::map< uint32_t, leveldown::Iterator * > iterators;

  static void WriteDoing(uv_work_t *req);
  static void WriteAfter(uv_work_t *req);

  static NAN_METHOD(New);
  static NAN_METHOD(Write);
  static NAN_METHOD(Iterator);
  static NAN_METHOD(GetProperty);
  static NAN_METHOD(ApproximateSizeSync);
  static NAN_METHOD(OpenSync);
  static NAN_METHOD(PutSync);
  static NAN_METHOD(DeleteSync);
  static NAN_METHOD(BatchSync);
  static NAN_METHOD(GetSync);
  static NAN_METHOD(IsExistsSync);
  static NAN_METHOD(MultiGetSync);
  static NAN_METHOD(CloseSync);
  static NAN_METHOD(GetBufferSync);
  static NAN_METHOD(CompactRangeSync);
};

} // namespace leveldown

#endif
