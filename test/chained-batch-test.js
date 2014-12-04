const test       = require('tap').test
    , testCommon = require('abstract-nosql/testCommon')
    , leveldown  = require('../')
    , abstract   = require('abstract-nosql/abstract/chained-batch-test')

if (require.main === module)
  abstract.all(leveldown, test, testCommon)
