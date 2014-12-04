const test      = require('tap').test
    , leveldown = require('../')
    , abstract  = require('abstract-nosql/abstract/leveldown-test')

if (require.main === module)
  abstract.args(leveldown, test)
