const test       = require('tap').test
    , testCommon = require('abstract-nosql/testCommon')
    , leveldown  = require('../')
    , abstract   = require('abstract-nosql/abstract/iterator-test')

if (require.main === module) {
  abstract.all(leveldown, test, testCommon)
}
