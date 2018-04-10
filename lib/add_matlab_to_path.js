
module.exports = function () {

  const find_command = require('./find_command');
  const path = require('path');

  // find MATLAB executable
  let matlab_path = which.sync('matlab',{nothrow:true});
  if (!matlab_path) {
    return new Error("MATLAB executable not found.");
  }
}