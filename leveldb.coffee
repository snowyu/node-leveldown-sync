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

  ### ## the native close async maybe crash ...
  _close: (cb)->
    return @binding.close(cb)
  ###

  _isExistsSync: (key, options) ->
    # fillCache = true
    # if typeof options == 'object'
    #   if options.fillCache == false
    #     fillCache = false
    result = @binding.isExistsSync(key, options)
    result

  _mGetSync: (keys, options) ->
    # fillCache = true
    asBuffer = false
    # needKeyName = true
    # raiseError = true
    if typeof options == 'object'
      # if options.fillCache == false
      #   fillCache = false
      if options.asBuffer == true
        asBuffer = true
      # if options.keys == false
      #   needKeyName = false
      # if options.raiseError == false
      #   raiseError = false
    result = @binding.mGetSync(keys, options)
    if asBuffer
      i = 1
      while i < result.length
        result[i] = new Buffer(result[i])
        i += 2
    result

  _getBufferSync: (key, destBuffer, options) ->
    # fillCache = true
    # offset = 0
    # if typeof options == 'object'
    #   if options.fillCache == false
    #     fillCache = false
    #   if options.offset > 0
    #     offset = options.offset
    result = @binding.getBufferSync(key, destBuffer, options)
    result

  _getSync: (key, options) ->
    # fillCache = true
    asBuffer = false
    if typeof options == 'object'
      # if options.fillCache == false
      #   fillCache = false
      if options.asBuffer == true
        asBuffer = true
    result = @binding.getSync(key, options)
    if asBuffer
      result = new Buffer(result)
    result

  _putSync: (key, value, options) ->
    # flushSync = false
    # if typeof options == 'object' and options.sync == true
    #   flushSync = true
    @binding.putSync key, value, options

  _delSync: (key, options) ->
    # flushSync = false
    # if typeof options == 'object' and options.sync == true
    #   flushSync = true
    @binding.delSync key, options

  _batchSync: (operations, options) ->
    # flushSync = false
    # if typeof options == 'object' and options.sync == true
    #   flushSync = true
    @binding.batchSync operations, options

  _approximateSizeSync: (start, end) ->
    @binding.approximateSizeSync start, end

  compactRangeSync: (start, end) ->
    @binding.compactRangeSync start, end

  compactRangeAsync: (start, end, callback) ->
    that = @
    setImmediate ->
      result = undefined
      try
        result = that.compactRangeSync(start, end)
      catch err
        callback err
        return
      callback null, result
      return

  compactRange: (start, end, callback) ->
    if typeof callback != 'function'
      @compactRangeSync start, end
    else
      @compactRangeAsync start, end, callback

  _chainedBatch: -> new ChainedBatch(this)

  getProperty: (property) ->
    if typeof property != 'string'
      throw new InvalidArgumentError('getProperty() requires a valid `property` argument')
    @binding.getProperty property

  IteratorClass: Iterator


  @destroySync: (location) -> binding.destroySync location
  @repairSync: (location) -> binding.repairSync location

  @repairAsync: (location, callback) ->
    that = @
    setImmediate ->
      result = undefined
      try
        result = that.repairSync(location)
      catch err
        callback err
        return
      callback null, result
      return

  @destroyAsync: (location, callback) ->
    that = @
    setImmediate ->
      result = undefined
      try
        result = that.destroySync(location)
      catch err
        callback err
        return
      callback null, result
      return

  @destroy: (location, callback) ->
    if typeof location != 'string'
      throw new InvalidArgumentError('destroy() requires a location string argument')
    if typeof callback != 'function'
      @destroySync location
    else
      @destroyAsync location, callback

  @repair: (location, callback) ->
    if typeof location != 'string'
      throw new InvalidArgumentError('repair() requires a location string argument')
    if typeof callback != 'function'
      @repairSync location
    else
      @repairAsync location, callback

module.exports = LevelDB
