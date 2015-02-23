const test       = require('tap').test
    , testCommon = require('abstract-nosql/testCommon')
    , eventable  = require('events-ex/eventable')
    , leveldown  = eventable(require('../'))
    , abstract   = require('abstract-nosql/abstract/event-test')

if (require.main === module)
  abstract.all(leveldown, test, testCommon)
