/**
 * \brief Installing matlab-engine package
 *
 * 1. detect matlab
 * 2. build native addon binary
 */

const {spawn} = require('child_process');
const which = require('which');

if (!which.sync('matlab', {nothrow: true})) 
  throw new Error('A Matlab executable must be in the system path.');

// ...

new Promise(function (resolve, reject) {
  var child = spawn('./node_modules/.bin/ncmake', ['rebuild'],{stdio:'inherit'});

  child
    .stdout
    .on('data', function (data) {
      process
        .stdout
        .write(data);
    });

  child.on('error', function (data) {
    reject('ncmake errored!');
  });

  child.on('exit', function () {
    resolve('ncmake completed successfully');
  });
})
  .then(function (v) {
    console.log(v);
  }, function (r) {
    console.error(r);
  });
