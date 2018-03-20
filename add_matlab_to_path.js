module.exports = function () {

  const find_command = require('./find_command');
  const path = require('path');

  // find MATLAB executable
  let res = find_command('matlab');
  if (!res.found) {
    return new Error("MATLAB executable not found.")
  }

  // get the directory
  matlab_dir = path.dirname(res.path);

  // specific for Windows version to get to the actual bin directory
  if (process.platform === 'win32') { // go to win64 subdirectory
    matlab_dir = path.join(matlab_dir, 'win64');
    const fs = require('fs');
    fs.access(matlab_dir, fs.constants.R_OK, (err) => {
      if (err) 
        matlab_dir = '';
      }
    );

    if (process.env.PATH.search(matlab_dir) < 0) 
    {
      process.env.PATH += path.delimiter + matlab_dir;
    }
  }
  return null;
}
