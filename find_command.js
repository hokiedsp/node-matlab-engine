module.exports = function (command) {

  // make sure it's a string
  if (typeof(command) !== 'string' || command === '') 
    return {found: false, dir: ''};
  
  // decide on which command to use
  var where;
  var is_win = false;
  switch (process.platform) {
    case 'win32':
      where = 'where';
      is_win = true;
      break;
    case 'darwin':
      where = 'which';
      break;
    default:
      where = 'whereis';
  }

  // run the command
  const execSync = require('child_process').execSync;
  try {
    var command_dir = execSync(where + ' ' + command, {
      stdio: 'pipe',
      encoding: 'utf8'
    });
  } catch (error) {
    return {found: false, dir: ''};
  }

  return {
    found: (command_dir !== ''),
    path: command_dir
  };
}
