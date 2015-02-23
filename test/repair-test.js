const test         = require('tap').test
    , fs           = require('fs')
    , path         = require('path')
    , mkfiletree   = require('mkfiletree')
    , readfiletree = require('readfiletree')
    , testCommon   = require('abstract-nosql/testCommon')
    , AbstractError= require('abstract-nosql/abstract-error').AbstractError
    , leveldown    = require('../')
    , makeTest     = require('./make-test')

test('test argument-less repair() throws', function (t) {
  t.throws(
      leveldown.repair
    , { name: 'InvalidArgumentError', message: 'repair() requires a location string argument' }
    , 'no-arg repair() throws'
  )
  t.end()
})

test('test repair non-existant directory returns error sync', function (t) {
  t.throws(
      leveldown.repair.bind(null, '/1/2/3/4')
    , { name: 'Error', message: 'IO error: /1/2/3/4: No such file or directory' }
    , 'callback-less, 1-arg repair() throws'
  )
  t.end()
})

test('test repair non-existant directory returns error', function (t) {
  leveldown.repair('/1/2/3/4', function (err) {
    console.log(err.message)
    t.ok((/no such/i).test(err.message), 'error on callback')
    t.equal(err.code, AbstractError.IO)
    t.end()
  })
})

// a proxy indicator that RepairDB is being called and doing its thing
makeTest('test repair() compacts', function (db, t, done, location) {
  db.close(function (err) {
    t.notOk(err, 'no error')
    var files = fs.readdirSync(location)
    t.ok(files.some(function (f) { return (/\.log$/).test(f) }), 'directory contains log file(s)')
    t.notOk(files.some(function (f) { return (/\.ldb$/).test(f) }), 'directory does not contain ldb file(s)')
    leveldown.repair(location, function () {
      files = fs.readdirSync(location)
      t.notOk(files.some(function (f) { return (/\.log$/).test(f) }), 'directory does not contain log file(s)')
      t.ok(files.some(function (f) { return (/\.ldb$/).test(f) }), 'directory contains ldb file(s)')
      done(false)
    })
  })
})

makeTest('test repair() compacts sync', function (db, t, done, location) {
  db.close(function (err) {
    t.notOk(err, 'no error')
    var files = fs.readdirSync(location)
    t.ok(files.some(function (f) { return (/\.log$/).test(f) }), 'directory contains log file(s)')
    t.notOk(files.some(function (f) { return (/\.ldb$/).test(f) }), 'directory does not contain ldb file(s)')
    leveldown.repair(location)
    files = fs.readdirSync(location)
    t.notOk(files.some(function (f) { return (/\.log$/).test(f) }), 'directory does not contain log file(s)')
    t.ok(files.some(function (f) { return (/\.ldb$/).test(f) }), 'directory contains ldb file(s)')
    done(false)
  })
})
