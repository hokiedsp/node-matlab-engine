
let add_matlab_to_path = require('../add_matlab_to_path');
if (add_matlab_to_path())
{
  console.log('MATLAB is not found. Cannot run the app.');
  return;
}

var matlab = require('bindings')('addon');

console.log(matlab);

var obj = new matlab.Engine();
console.log(obj);
// var o = obj;
// do {
//     console.log(Object.getOwnPropertyNames(o));
// } while (o = Object.getPrototypeOf(o));
obj.evaluate("x = 5");
// var mxvar = obj.getData("x");
// console.log( mxvar.getData() ); // 11

delete obj

function forceGC() {
  if (global.gc) {
    global.gc();
  } else {
    console.warn('No GC hook! Start your program as `node --expose-gc file.js`.');
  }
}

forceGC();
