
let add_matlab_to_path = require('../add_matlab_to_path');
if (add_matlab_to_path())
{
  console.log('MATLAB is not found. Cannot run the app.');
  return;
}

var matlab = require('bindings')('addon');

console.log(matlab);
console.log(matlab.Engine);
console.log(matlab.MxArray);

var obj = new matlab.Engine();
obj.evaluate("x = 5");
// var mxvar = obj.getData("x");
// console.log( mxvar.getData() ); // 11
