const {Engine:matlab} = require('../index.js');

var session = new matlab();

console.log('session=');
console.log(JSON.stringify(session));

// var o = session; do {     console.log(Object.getOwnPropertyNames(o)); } while
// (o = Object.getPrototypeOf(o));

session.bufferEnabled = false;
session.visible = false;

console.log('session.visible=' + session.visible);
console.log('session.buffer=' + session.buffer);
console.log('session.bufferEnabled=' + session.bufferEnabled);
console.log('session.bufferSize=' + session.bufferSize);

console.log('Pause 3 seconds before closing Matlab');
setTimeout(() => {
  session.visible = true;
  console.log('session.visible=' + session.visible);

  session.close();
  console.log('Matlab closed');


  delete session;
  console.log('Matlab object deleted');
}, 3000);

function forceGC() {
  if (global.gc) {
    global.gc();
  } else {
    console.warn('No GC hook! Start your program as `node --expose-gc file.js`.');
  }
}

forceGC();
