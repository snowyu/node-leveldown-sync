const test       = require('tap').test
    , testCommon = require('abstract-nosql/testCommon')
    , leveldown  = require('../')
    , abstract   = require('abstract-nosql/abstract/ranges-test')

if (require.main === module)
  abstract.all(leveldown, test, testCommon)
