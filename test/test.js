const {Engine: Matlab} = require('../index.js');

var session = new Matlab();

console.log(session);
// var o = session; do {     console.log(Object.getOwnPropertyNames(o)); } while
// (o = Object.getPrototypeOf(o));
session.evalSync("x = 5");

console.log(session.visible
  ? "MATLAB is visible"
  : "MATLAB is hidden");
session.visible = false;
console.log(session.visible
  ? "MATLAB is visible"
  : "MATLAB is hidden");
session.visible = true;
console.log(session.visible
  ? "MATLAB is visible"
  : "MATLAB is hidden");

session.bufferSize = 2048;
console.log(session.evalSync("ver"));
session.evalSync("help");
console.log(session.buffer);

var x = session.getVariable("x");
console.log(x);

session.putVariable("y", "test string");
console.log(session.getVariable("y"));

session.evalSync("s = struct('x',x,'y',y)");
var s = session.getVariable("s");
console.log(s);
session.putVariable("s1",s);

session.close();
console.log('Matlab closed');

function forceGC() {
  if (global.gc) {
    global.gc();
  } else {
    console.warn('No GC hook! Start your program as `node --expose-gc file.js`.');
  }
}

forceGC();
