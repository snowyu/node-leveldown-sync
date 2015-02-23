inherits      = require('inherits-ex')
AbstractNoSQL = require('abstract-nosql')
Errors        = require('abstract-nosql/abstract-error')
binding       = require('bindings')('leveldown.node').leveldown
ChainedBatch  = require('./chained-batch')
Iterator      = require('./iterator')
InvalidArgumentError = Errors.InvalidArgumentError

class LevelDB 
  inherits LevelDB, AbstractNoSQL
  constructor: (location) ->
    return new LevelDB(location) if !(this instanceof LevelDB)
    super(location)
    @binding = binding(location)
    return


  _openSync: (options) ->
    @binding.openSync options

  _closeSync: ->
    @binding.closeSync()

  _isExistsSync: (key, options) ->
    fillCache = true
    if typeof options == 'object'
      if options.fillCache == false
        fillCache = false
    result = @binding.isExistsSync(key, fillCache)
    result

  _mGetSync: (keys, options) ->
    fillCache = true
    asBuffer = false
    needKeyName = true
    raiseError = true
    if typeof options == 'object'
      if options.fillCache == false
        fillCache = false
      if options.asBuffer == true
        asBuffer = true
      if options.keys == false
        needKeyName = false
      if options.raiseError == false
        raiseError = false
    result = @binding.mGetSync(keys, fillCache, needKeyName, raiseError)
    if asBuffer
      i = 1
      while i < result.length
        result[i] = new Buffer(result[i])
        i += 2
    result

  _getBufferSync: (key, destBuffer, options) ->
    fillCache = true
    offset = 0
    if typeof options == 'object'
      if options.fillCache == false
        fillCache = false
      if options.offset > 0
        offset = options.offset
    result = @binding.getBufferSync(key, destBuffer, fillCache, offset)
    result

  _getSync: (key, options) ->
    fillCache = true
    asBuffer = false
    if typeof options == 'object'
      if options.fillCache == false
        fillCache = false
      if options.asBuffer == true
        asBuffer = true
    result = @binding.getSync(key, fillCache)
    if asBuffer
      result = new Buffer(result)
    result

  _putSync: (key, value, options) ->
    flushSync = false
    if typeof options == 'object' and options.sync == true
      flushSync = true
    @binding.putSync key, value, flushSync

  _delSync: (key, options) ->
    flushSync = false
    if typeof options == 'object' and options.sync == true
      flushSync = true
    @binding.delSync key, flushSync

  _batchSync: (operations, options) ->
    flushSync = false
    if typeof options == 'object' and options.sync == true
      flushSync = true
    @binding.batchSync operations, flushSync

  _approximateSizeSync: (start, end) ->
    @binding.approximateSizeSync start, end

  _chainedBatch: -> new ChainedBatch(this)

  getProperty: (property) ->
    if typeof property != 'string'
      throw new InvalidArgumentError('getProperty() requires a valid `property` argument')
    @binding.getProperty property

  IteratorClass: Iterator


  @destroySync: (location) -> binding.destroySync location
  @repairSync: (location) -> binding.repairSync location
  @destroy: (location, callback) ->
    if typeof location != 'string'
      throw new InvalidArgumentError('destroy() requires a location string argument')
    if typeof callback != 'function'
      LevelDB.destroySync location
    else
      binding.destroy location, callback

  @repair: (location, callback) ->
    if typeof location != 'string'
      throw new InvalidArgumentError('repair() requires a location string argument')
    if typeof callback != 'function'
      LevelDB.repairSync location
    else
      binding.repair location, callback

module.exports = LevelDB
