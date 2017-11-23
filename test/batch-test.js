const tap        = require('tap')
    , test       = tap.test
    , testCommon = require('abstract-nosql/testCommon')
    , leveldown  = require('../')
    , abstract   = require('abstract-nosql/abstract/batch-test')

if (require.main === module)
  abstract.all(leveldown, test, testCommon)
