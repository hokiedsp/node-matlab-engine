const matlab = require('../index.js');

console.log(matlab.Engine);

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

var x = session.getVariable("x");
xdata = x.getData();
console.log( xdata ); // 5
console.log( x.getDimensions());

xdata[0] = 4;
console.log( x.getData() ); // 5

x = session.getVariableValue("x");
console.log(x);

let y = new matlab.MxArray();
y.setData("test string");
session.putVariable("y", y);
console.log( session.getVariableValue("y") ); // 5

session.putVariableValue("y", "test string assigning by value");
console.log( session.getVariableValue("y") ); // 5

session.evaluate("s = struct('x',x,'y',y)");
console.log( session.getVariableValue("s") ); // 5

delete x;

// session.close();
console.log('Matlab closed');
delete session;
console.log('Matlab destroyed');

function forceGC() {
  if (global.gc) {
    global.gc();
  } else {
    console.warn('No GC hook! Start your program as `node --expose-gc file.js`.');
  }
}

forceGC();
