// This tests an equals import of an external module reference that uses the
// export equals syntax.

//- @obj ref/imports Def
//- @import defines/binding LocalObj
//- LocalObj.node/kind variable
//- LocalObj.subkind import
//- LocalObj aliases Def
import obj = require('./equals_export');

//- @obj ref LocalObj
obj.key;
