"use strict";

const which = require('which');
const path = require('path');

// find MATLAB executable
let matlab_exe;
try {
  matlab_exe = which.sync('matlab');
} catch {throw 'MATLAB executable not found.';}

// specific for Windows version to get to the actual bin directory
if (process.platform === 'win32') { // go to win64 subdirectory
  // get the directory
  let matlab_dir = path.dirname(matlab_exe);

  matlab_dir = path.join(matlab_dir, 'win64');
  const fs = require('fs');
  try {
    fs.accessSync(matlab_dir, fs.constants.R_OK)
  } catch {throw 'MATLAB bin folder not found.';}

  if (process.env.PATH.search(matlab_dir) < 0) {
    // add only if the path is not already included
    process.env.PATH += path.delimiter + matlab_dir;
    console.log('"added "' + matlab_dir + '" to path.');
  }
}

module.exports = require("bindings")("addon.node");