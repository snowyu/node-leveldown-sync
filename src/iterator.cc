/* Copyright (c) 2012-2017 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>
#include <node_buffer.h>

#include "database.h"
#include "iterator.h"
#include "common.h"

namespace leveldown {

static Nan::Persistent<v8::FunctionTemplate> iterator_constructor;

Iterator::Iterator (
    Database* database
  , uint32_t id
  , leveldb::Slice* start
  , std::string* end
  , bool reverse
  , bool keys
  , bool values
  , int limit
  , std::string* lt
  , std::string* lte
  , std::string* gt
  , std::string* gte
  , bool fillCache
  , bool keyAsBuffer
  , bool valueAsBuffer
  , size_t highWaterMark
) : database(database)
  , id(id)
  , start(start)
  , end(end)
  , reverse(reverse)
  , keys(keys)
  , values(values)
  , limit(limit)
  , lt(lt)
  , lte(lte)
  , gt(gt)
  , gte(gte)
  , highWaterMark(highWaterMark)
  , keyAsBuffer(keyAsBuffer)
  , valueAsBuffer(valueAsBuffer)
{
  Nan::HandleScope scope;

  options    = new leveldb::ReadOptions();
  options->fill_cache = fillCache;
  // get a snapshot of the current state
  options->snapshot = database->NewSnapshot();
  dbIterator = NULL;
  count      = 0;
  target     = NULL;
  seeking    = false;
  landed     = false;
  nexting    = false;
  ended      = false;
};

Iterator::~Iterator () {
  // printf("\ndestroy Iterator:%d,%d\n", id, ended);
  if (TryLockEnd()) { //else already Closing
    IteratorEnd();
    Release();
    UnlockEnd();
  }
  // printf("\ndestroy Iterator:free snapshot ok\n");

  delete options;
  ReleaseTarget();
  if (start != NULL) {
    // Special case for `start` option: it won't be
    // freed up by any of the delete calls below.
    if (!((lt != NULL && reverse)
        || (lte != NULL && reverse)
        || (gt != NULL && !reverse)
        || (gte != NULL && !reverse))) {
      delete[] start->data();
    }
    delete start;
  }
  if (end != NULL)
    delete end;
  if (lt != NULL)
    delete lt;
  if (gt != NULL)
    delete gt;
  if (lte != NULL)
    delete lte;
  if (gte != NULL)
    delete gte;
};

void Iterator::IteratorEnd () {
  // printf("\ndestroy dbIterator:%d\n", dbIterator);
  ended = true;
  //TODO: could return it->status()
  delete dbIterator;
  dbIterator = NULL;
}

void Iterator::Release () {
  if (options->snapshot) {
    // printf("\nIterator::Release:%d\n", id);
    database->ReleaseSnapshot(options->snapshot);
    options->snapshot = NULL;
    database->ReleaseIterator(id);
  }
  // printf("\nIterator::Release:%d done\n", id);
}

void Iterator::ReleaseTarget () {
  if (target != NULL) {

    if (!target->empty())
      delete[] target->data();

    delete target;
    target = NULL;
  }
}

void Iterator::Close () {
  // endLocker.lock();
  if (!ended) {
    // ended = true;
    IteratorEnd();
    Release();
  }
  // endLocker.unlock();
}

inline bool Iterator::TryLockEnd () {
  return !ended ;//&& endLocker.try_lock();
}

inline void Iterator::UnlockEnd () {
  // endLocker.unlock();
}

bool Iterator::GetIterator () {
  if (dbIterator == NULL) {
    dbIterator = database->NewIterator(options);

    if (start != NULL) {
      dbIterator->Seek(*start);

      if (reverse) {
        if (!dbIterator->Valid()) {
          // if it's past the last key, step back
          dbIterator->SeekToLast();
        } else {
          std::string key_ = dbIterator->key().ToString();

          if (lt != NULL) {
            if (lt->compare(key_) <= 0)
              dbIterator->Prev();
          } else if (lte != NULL) {
            if (lte->compare(key_) < 0)
              dbIterator->Prev();
          } else if (start != NULL) {
            if (start->compare(key_))
              dbIterator->Prev();
          }
        }

        if (dbIterator->Valid() && lt != NULL) {
          if (lt->compare(dbIterator->key().ToString()) <= 0)
            dbIterator->Prev();
        }
      } else {
        if (dbIterator->Valid() && gt != NULL
            && gt->compare(dbIterator->key().ToString()) == 0)
          dbIterator->Next();
      }
    } else if (reverse) {
      dbIterator->SeekToLast();
    } else {
      dbIterator->SeekToFirst();
    }

    return true;
  }
  return false;
}

bool Iterator::Read (std::string& key, std::string& value) {
  // if it's not the first call, move to next item.
  if (!GetIterator() && !seeking) {
    if (reverse)
      dbIterator->Prev();
    else
      dbIterator->Next();
  }

  seeking = false;

  // now check if this is the end or not, if not then return the key & value
  if (dbIterator->Valid()) {
    std::string key_ = dbIterator->key().ToString();
    int isEnd = end == NULL ? 1 : end->compare(key_);

    if ((limit < 0 || ++count <= limit)
      && (end == NULL
          || (reverse && (isEnd <= 0))
          || (!reverse && (isEnd >= 0)))
      && ( lt  != NULL ? (lt->compare(key_) > 0)
         : lte != NULL ? (lte->compare(key_) >= 0)
         : true )
      && ( gt  != NULL ? (gt->compare(key_) < 0)
         : gte != NULL ? (gte->compare(key_) <= 0)
         : true )
    ) {
      if (keys)
        key.assign(dbIterator->key().data(), dbIterator->key().size());
      if (values)
        value.assign(dbIterator->value().data(), dbIterator->value().size());
      return true;
    }
  }

  return false;
}


// bool Iterator::Read (std::string& key, std::string& value) {
//   // if it's not the first call, move to next item.
//   if (!GetIterator() && !seeking) {
//     if (reverse)
//       dbIterator->Prev();
//     else
//       dbIterator->Next();
//   }

//   seeking = false;
//   bool result = dbIterator->Valid() && (limit < 0 || ++count <= limit);

//   // now check if this is the end or not, if not then return the key & value
//   if (result) {
//     std::string key_ = dbIterator->key().ToString();
//     int isEnd = end == NULL ? 1 : end->compare(key_);

//     result = (reverse ? (skipEnd ? isEnd <= 0 : isEnd < 0) : (skipEnd ? isEnd >= 0 : isEnd > 0));

//     if (result) {
//       if (keys)
//         key.assign(dbIterator->key().data(), dbIterator->key().size());
//       if (values)
//         value.assign(dbIterator->value().data(), dbIterator->value().size());
//     }
//   }
//   return result;
// }

bool Iterator::OutOfRange (leveldb::Slice* target) {
  if (lt != NULL) {
    if (target->compare(*lt) >= 0)
      return true;
  } else if (lte != NULL) {
    if (target->compare(*lte) > 0)
      return true;
  } else if (start != NULL && reverse) {
    if (target->compare(*start) > 0)
      return true;
  }

  if (end != NULL) {
    int d = target->compare(*end);
    if (reverse ? d < 0 : d > 0)
      return true;
  }

  if (gt != NULL) {
    if (target->compare(*gt) <= 0)
      return true;
  } else if (gte != NULL) {
    if (target->compare(*gte) < 0)
      return true;
  } else if (start != NULL && !reverse) {
    if (target->compare(*start) < 0)
      return true;
  }

  return false;
}

bool Iterator::IteratorNext (std::vector<std::pair<std::string, std::string> >& result) {
  size_t size = 0;
  while(true) {
    std::string key, value;
    bool ok = Read(key, value);

    if (ok) {
      result.push_back(std::make_pair(key, value));

      if (!landed) {
        landed = true;
        return true;
      }

      size = size + key.size() + value.size();
      if (size > highWaterMark)
        return true;

    } else {
      return false;
    }
  }
}

leveldb::Status Iterator::IteratorStatus () {
  return dbIterator->status();
}

//nextSync()
//return the array(2),
//  the first is the result array,
//  the second is the count of the result. if count <=0 means no more data.
NAN_METHOD(Iterator::NextSync) {
  Iterator* iterator = Nan::ObjectWrap::Unwrap<Iterator>(info.This());

  if (!iterator->TryLockEnd()) {
    return Nan::ThrowError("iterator has ended");
  }

  iterator->nexting = true;
  std::vector<std::pair<std::string, std::string> > result;
  bool ok = iterator->IteratorNext(result);
  iterator->ReleaseTarget();
  iterator->nexting = false;
  // checkEndCallback(iterator); //clean up & handle the next/end state
  // iterator->endLocker.unlock();
  if (!ok) {
    leveldb::Status status = iterator->IteratorStatus();
    LD_METHOD_CHECK_DB_ERROR(nextSync);
  }

  size_t idx = 0;

  size_t arraySize = result.size() * 2;
  v8::Local<v8::Array> returnArray = Nan::New<v8::Array>(arraySize);

  for(idx = 0; idx < result.size(); ++idx) {
    std::pair<std::string, std::string> row = result[idx];
    std::string key = row.first;
    std::string value = row.second;

    v8::Local<v8::Value> returnKey;
    if (iterator->keyAsBuffer) {
      //TODO: use NewBuffer, see database_async.cc
      returnKey = Nan::CopyBuffer((char*)key.data(), key.size()).ToLocalChecked();
    } else {
      returnKey = Nan::New<v8::String>((char*)key.data(), key.size()).ToLocalChecked();
    }

    v8::Local<v8::Value> returnValue;
    if (iterator->valueAsBuffer) {
      //TODO: use NewBuffer, see database_async.cc
      returnValue = Nan::CopyBuffer((char*)value.data(), value.size()).ToLocalChecked();
    } else {
      returnValue = Nan::New<v8::String>((char*)value.data(), value.size()).ToLocalChecked();
    }

    // put the key & value in a descending order, so that they can be .pop:ed in javascript-land
    returnArray->Set(Nan::New<v8::Integer>(static_cast<int>(arraySize - idx * 2 - 1)), returnKey);
    returnArray->Set(Nan::New<v8::Integer>(static_cast<int>(arraySize - idx * 2 - 2)), returnValue);
  }


  ssize_t s =  result.size();
  if (!ok) s = -s;
  v8::Local<v8::Array> returnResult = Nan::New<v8::Array>(2);
  returnResult->Set(Nan::New<v8::Integer>(0), returnArray);
  // when size is negated, all data has been read, so it's then finished
  returnResult->Set(Nan::New<v8::Integer>(1), Nan::New<v8::Integer>(static_cast<int>(s)));
  info.GetReturnValue().Set(returnResult);
}

NAN_METHOD(Iterator::EndSync) {
  Iterator* iterator = Nan::ObjectWrap::Unwrap<Iterator>(info.This());
  bool result = true;

  iterator->Close();

  info.GetReturnValue().Set(result);
}

NAN_METHOD(Iterator::Seek) {
  Iterator* iterator = Nan::ObjectWrap::Unwrap<Iterator>(info.This());

  if (!iterator->TryLockEnd()) {
    return Nan::ThrowError("iterator has ended");
  }

  iterator->ReleaseTarget();

  v8::Local<v8::Value> targetBuffer = info[0].As<v8::Value>();
  LD_STRING_OR_BUFFER_TO_COPY(_target, targetBuffer, target);
  iterator->target = new leveldb::Slice(_targetCh_, _targetSz_);

  iterator->GetIterator();
  leveldb::Iterator* dbIterator = iterator->dbIterator;

  dbIterator->Seek(*iterator->target);
  iterator->seeking = true;
  iterator->landed = false;

  if (iterator->OutOfRange(iterator->target)) {
    if (iterator->reverse) {
      dbIterator->SeekToFirst();
      dbIterator->Prev();
    } else {
      dbIterator->SeekToLast();
      dbIterator->Next();
    }
  }
  else {
    if (dbIterator->Valid()) {
      int cmp = dbIterator->key().compare(*iterator->target);
      if (cmp > 0 && iterator->reverse) {
        dbIterator->Prev();
      } else if (cmp < 0 && !iterator->reverse) {
        dbIterator->Next();
      }
    } else {
      if (iterator->reverse) {
        dbIterator->SeekToLast();
      } else {
        dbIterator->SeekToFirst();
      }
      if (dbIterator->Valid()) {
        int cmp = dbIterator->key().compare(*iterator->target);
        if (cmp > 0 && iterator->reverse) {
          dbIterator->SeekToFirst();
          dbIterator->Prev();
        } else if (cmp < 0 && !iterator->reverse) {
          dbIterator->SeekToLast();
          dbIterator->Next();
        }
      }
    }
  }
  // iterator->endLocker.unlock();

  info.GetReturnValue().Set(info.Holder());
}

void Iterator::Init () {
  v8::Local<v8::FunctionTemplate> tpl =
      Nan::New<v8::FunctionTemplate>(Iterator::New);
  iterator_constructor.Reset(tpl);
  tpl->SetClassName(Nan::New("Iterator").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  Nan::SetPrototypeMethod(tpl, "seek", Iterator::Seek);
  Nan::SetPrototypeMethod(tpl, "endSync", Iterator::EndSync);
  Nan::SetPrototypeMethod(tpl, "nextSync", Iterator::NextSync);
}

v8::Local<v8::Object> Iterator::NewInstance (
        v8::Local<v8::Object> database
      , v8::Local<v8::Number> id
      , v8::Local<v8::Object> optionsObj
    ) {

  Nan::EscapableHandleScope scope;

  Nan::MaybeLocal<v8::Object> maybeInstance;
  v8::Local<v8::Object> instance;
  v8::Local<v8::FunctionTemplate> constructorHandle =
      Nan::New<v8::FunctionTemplate>(iterator_constructor);

  if (optionsObj.IsEmpty()) {
    v8::Local<v8::Value> argv[2] = { database, id };
    maybeInstance = Nan::NewInstance(constructorHandle->GetFunction(), 2, argv);
  } else {
    v8::Local<v8::Value> argv[3] = { database, id, optionsObj };
    maybeInstance = Nan::NewInstance(constructorHandle->GetFunction(), 3, argv);
  }

  if (maybeInstance.IsEmpty())
      Nan::ThrowError("Could not create new Iterator instance");
  else
    instance = maybeInstance.ToLocalChecked();
  return scope.Escape(instance);
}

NAN_METHOD(Iterator::New) {
  Database* database = Nan::ObjectWrap::Unwrap<Database>(info[0]->ToObject());

  leveldb::Slice* start = NULL;
  std::string* end = NULL;
  int limit = -1;
  // default highWaterMark from Readble-streams
  size_t highWaterMark = 16 * 1024;

  v8::Local<v8::Value> id = info[1];

  v8::Local<v8::Object> optionsObj;

  v8::Local<v8::Object> ltHandle;
  v8::Local<v8::Object> lteHandle;
  v8::Local<v8::Object> gtHandle;
  v8::Local<v8::Object> gteHandle;

  char *startStr = NULL;
  std::string* lt = NULL;
  std::string* lte = NULL;
  std::string* gt = NULL;
  std::string* gte = NULL;

  //default to forward.
  bool reverse = false;

  if (info.Length() > 1 && info[2]->IsObject()) {
    optionsObj = v8::Local<v8::Object>::Cast(info[2]);

    reverse = BooleanOptionValue(optionsObj, "reverse");

    if (optionsObj->Has(Nan::New("start").ToLocalChecked())
        && (node::Buffer::HasInstance(optionsObj->Get(Nan::New("start").ToLocalChecked()))
          || optionsObj->Get(Nan::New("start").ToLocalChecked())->IsString())) {

      v8::Local<v8::Value> startBuffer = optionsObj->Get(Nan::New("start").ToLocalChecked());

      // ignore start if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(startBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_COPY(_start, startBuffer, start)
        start = new leveldb::Slice(_startCh_, _startSz_);
        startStr = _startCh_;
      }
    }

    if (optionsObj->Has(Nan::New("end").ToLocalChecked())
        && (node::Buffer::HasInstance(optionsObj->Get(Nan::New("end").ToLocalChecked()))
          || optionsObj->Get(Nan::New("end").ToLocalChecked())->IsString())) {

      v8::Local<v8::Value> endBuffer = optionsObj->Get(Nan::New("end").ToLocalChecked());

      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(endBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_COPY(_end, endBuffer, end)
        end = new std::string(_endCh_, _endSz_);
        delete[] _endCh_;
      }
    }

    if (!optionsObj.IsEmpty() && optionsObj->Has(Nan::New("limit").ToLocalChecked())) {
      limit = v8::Local<v8::Integer>::Cast(optionsObj->Get(
          Nan::New("limit").ToLocalChecked()))->Value();
    }

    if (optionsObj->Has(Nan::New("highWaterMark").ToLocalChecked())) {
      highWaterMark = v8::Local<v8::Integer>::Cast(optionsObj->Get(
            Nan::New("highWaterMark").ToLocalChecked()))->Value();
    }

    if (optionsObj->Has(Nan::New("lt").ToLocalChecked())
        && (node::Buffer::HasInstance(optionsObj->Get(Nan::New("lt").ToLocalChecked()))
          || optionsObj->Get(Nan::New("lt").ToLocalChecked())->IsString())) {

      v8::Local<v8::Value> ltBuffer = optionsObj->Get(Nan::New("lt").ToLocalChecked());

      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(ltBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_COPY(_lt, ltBuffer, lt)
        lt = new std::string(_ltCh_, _ltSz_);
        delete[] _ltCh_;
        if (reverse) {
          if (startStr != NULL) {
            delete[] startStr;
            startStr = NULL;
          }
          if (start != NULL)
            delete start;
          start = new leveldb::Slice(lt->data(), lt->size());
        }
      }
    }

    if (optionsObj->Has(Nan::New("lte").ToLocalChecked())
        && (node::Buffer::HasInstance(optionsObj->Get(Nan::New("lte").ToLocalChecked()))
          || optionsObj->Get(Nan::New("lte").ToLocalChecked())->IsString())) {

      v8::Local<v8::Value> lteBuffer = optionsObj->Get(Nan::New("lte").ToLocalChecked());

      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(lteBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_COPY(_lte, lteBuffer, lte)
        lte = new std::string(_lteCh_, _lteSz_);
        delete[] _lteCh_;
        if (reverse) {
          if (startStr != NULL) {
            delete[] startStr;
            startStr = NULL;
          }
          if (start != NULL)
            delete start;
          start = new leveldb::Slice(lte->data(), lte->size());
        }
      }
    }

    if (optionsObj->Has(Nan::New("gt").ToLocalChecked())
        && (node::Buffer::HasInstance(optionsObj->Get(Nan::New("gt").ToLocalChecked()))
          || optionsObj->Get(Nan::New("gt").ToLocalChecked())->IsString())) {

      v8::Local<v8::Value> gtBuffer = optionsObj->Get(Nan::New("gt").ToLocalChecked());

      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(gtBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_COPY(_gt, gtBuffer, gt)
        gt = new std::string(_gtCh_, _gtSz_);
        delete[] _gtCh_;
        if (!reverse) {
          if (startStr != NULL) {
            delete[] startStr;
            startStr = NULL;
          }
          if (start != NULL)
            delete start;
          start = new leveldb::Slice(gt->data(), gt->size());
        }
      }
    }

    if (optionsObj->Has(Nan::New("gte").ToLocalChecked())
        && (node::Buffer::HasInstance(optionsObj->Get(Nan::New("gte").ToLocalChecked()))
          || optionsObj->Get(Nan::New("gte").ToLocalChecked())->IsString())) {

      v8::Local<v8::Value> gteBuffer = optionsObj->Get(Nan::New("gte").ToLocalChecked());

      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(gteBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_COPY(_gte, gteBuffer, gte)
        gte = new std::string(_gteCh_, _gteSz_);
        delete[] _gteCh_;
        if (!reverse) {
          if (startStr != NULL) {
            delete[] startStr;
            startStr = NULL;
          }
          if (start != NULL)
            delete start;
          start = new leveldb::Slice(gte->data(), gte->size());
        }
      }
    }

  }

  bool keys = BooleanOptionValue(optionsObj, "keys", true);
  bool values = BooleanOptionValue(optionsObj, "values", true);
  bool keyAsBuffer = BooleanOptionValue(optionsObj, "keyAsBuffer", true);
  bool valueAsBuffer = BooleanOptionValue(optionsObj, "valueAsBuffer", true);
  bool fillCache = BooleanOptionValue(optionsObj, "fillCache");

  Iterator* iterator = new Iterator(
      database
    , (uint32_t)id->Int32Value()
    , start
    , end
    , reverse
    , keys
    , values
    , limit
    , lt
    , lte
    , gt
    , gte
    , fillCache
    , keyAsBuffer
    , valueAsBuffer
    , highWaterMark
  );
  iterator->Wrap(info.This());

  info.GetReturnValue().Set(info.This());
}

} // namespace leveldown
