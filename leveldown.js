const util              = require('util')
    , AbstractNoSQL     = require('abstract-nosql').AbstractNoSQL
    , Errors            = require('abstract-nosql/abstract-error')

    , binding           = require('bindings')('leveldown.node').leveldown

    , ChainedBatch      = require('./chained-batch')
    , Iterator          = require('./iterator')
    , InvalidArgumentError = Errors.InvalidArgumentError


function LevelDOWN (location) {
  if (!(this instanceof LevelDOWN))
    return new LevelDOWN(location)

  AbstractNoSQL.call(this, location)
  this.binding = binding(location)
}

util.inherits(LevelDOWN, AbstractNoSQL)

LevelDOWN.prototype._openSync = function (options) {
  return this.binding.openSync(options)
}

LevelDOWN.prototype._closeSync = function () {
  return this.binding.closeSync()
}

LevelDOWN.prototype._isExistsSync = function (key, options) {
  var fillCache = true;
  if (typeof options === 'object') {
    if (options.fillCache === false) fillCache = false;
  }
  var result = this.binding.isExistsSync(key, fillCache);
  return result;
}

LevelDOWN.prototype._mGetSync = function (keys, options) {
  var fillCache = true;
  var asBuffer = false;
  var needKeyName = true;
  var raiseError = true;
  if (typeof options === 'object') {
    if (options.fillCache === false) fillCache = false;
    if (options.asBuffer === true) asBuffer = true;
    if (options.keys === false) needKeyName = false;
    if (options.raiseError === false) raiseError = false;
  }
  var result = this.binding.mGetSync(keys, fillCache, needKeyName, raiseError);
  if (asBuffer) for (var i=1; i < result.length; i+=2) {
    result[i] = new Buffer(result[i]);
  }
  return result;
}

LevelDOWN.prototype._getBufferSync = function (key, destBuffer, options) {
  var fillCache = true;
  var offset = 0;
  if (typeof options === 'object') {
    if (options.fillCache === false) fillCache = false;
    if (options.offset > 0) offset = options.offset;
  }
  var result = this.binding.getBufferSync(key, destBuffer, fillCache, offset);
  return result;
}

LevelDOWN.prototype._getSync = function (key, options) {
  var fillCache = true;
  var asBuffer = false;
  if (typeof options === 'object') {
    if (options.fillCache === false) fillCache = false;
    if (options.asBuffer === true) asBuffer = true;
  }
  var result = this.binding.getSync(key, fillCache);
  if (asBuffer) result = new Buffer(result);
  return result;
}

LevelDOWN.prototype._putSync = function (key, value, options) {
  var flushSync = false;
  if (typeof options === 'object' && options.sync === true) flushSync = true;
  return this.binding.putSync(key, value, flushSync)
}

LevelDOWN.prototype._delSync = function (key, options) {
  var flushSync = false;
  if (typeof options === 'object' && options.sync === true) flushSync = true;
  return this.binding.delSync(key, flushSync)
}

LevelDOWN.prototype._batchSync = function (operations, options) {
  var flushSync = false;
  if (typeof options === 'object' && options.sync === true) flushSync = true;
  return this.binding.batchSync(operations, flushSync)
}

LevelDOWN.prototype._approximateSizeSync = function (start, end) {
  return this.binding.approximateSizeSync(start, end)
}


LevelDOWN.prototype._open = function (options, callback) {
  return this.binding.open(options, callback)
}

LevelDOWN.prototype._close = function (callback) {
  this.binding.close(callback)
}

LevelDOWN.prototype._put = function (key, value, options, callback) {
  return this.binding.put(key, value, options, callback)
}

LevelDOWN.prototype._get = function (key, options, callback) {
  return this.binding.get(key, options, callback)
}

LevelDOWN.prototype._del = function (key, options, callback) {
  return this.binding.del(key, options, callback)
}

LevelDOWN.prototype._chainedBatch = function () {
  return new ChainedBatch(this)
}


LevelDOWN.prototype._batch = function (operations, options, callback) {
  return this.binding.batch(operations, options, callback)
}


LevelDOWN.prototype._approximateSize = function (start, end, callback) {
  return this.binding.approximateSize(start, end, callback)
}


LevelDOWN.prototype.getProperty = function (property) {
  if (typeof property != 'string')
    throw new InvalidArgumentError('getProperty() requires a valid `property` argument')

  return this.binding.getProperty(property)
}

LevelDOWN.prototype.IteratorClass = Iterator

LevelDOWN.prototype._iterator = function (options) {
  return new Iterator(this, options)
}


LevelDOWN.destroy = function (location, callback) {
  if (arguments.length < 2)
    throw new InvalidArgumentError('destroy() requires `location` and `callback` arguments')

  if (typeof location != 'string')
    throw new InvalidArgumentError('destroy() requires a location string argument')

  if (typeof callback != 'function')
    throw new InvalidArgumentError('destroy() requires a callback function argument')

  binding.destroy(location, callback)
}


LevelDOWN.repair = function (location, callback) {
  if (arguments.length < 2)
    throw new InvalidArgumentError('repair() requires `location` and `callback` arguments')

  if (typeof location != 'string')
    throw new InvalidArgumentError('repair() requires a location string argument')

  if (typeof callback != 'function')
    throw new InvalidArgumentError('repair() requires a callback function argument')

  binding.repair(location, callback)
}


module.exports = LevelDOWN
