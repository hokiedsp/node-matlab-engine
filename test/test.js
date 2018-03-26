
let add_matlab_to_path = require('../add_matlab_to_path');
if (add_matlab_to_path())
{
  console.log('MATLAB is not found. Cannot run the app.');
  return;
}

var matlab = require('bindings')('addon');

console.log(matlab);

var session = new matlab.Engine();
console.log(session);
// var o = session;
// do {
//     console.log(Object.getOwnPropertyNames(o));
// } while (o = Object.getPrototypeOf(o));
session.evaluate("x = 5");

console.log(session.getVisible() ? "MATLAB is visible":"MATLAB is hidden");
session.setVisible(false);
console.log(session.getVisible() ? "MATLAB is visible":"MATLAB is hidden");
session.setVisible(true);
console.log(session.getVisible() ? "MATLAB is visible":"MATLAB is hidden");

session.setOutputBufferSize(2048);
console.log(session.evaluate("ver"));
session.evaluate("help");
console.log(session.getLastOutput());

// var mxvar = session.getData("x");
// console.log( mxvar.getData() ); // 11

delete session

function forceGC() {
  if (global.gc) {
    global.gc();
  } else {
    console.warn('No GC hook! Start your program as `node --expose-gc file.js`.');
  }
}

forceGC();
