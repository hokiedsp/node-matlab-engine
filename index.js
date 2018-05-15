"use strict";

module.exports = require("bindings")("src/addon.node");

// let lib = require("./lib");

// let _ = require("lodash"); let ext = require("./ext"); let Bluebird =
// require("bluebird"); let debug = require("debug")("af");

// let matlabEngine = null;

// let entry = module.exports = function () {
//   let ml = matlabEngine || (matlabEngine = require("bindings")("./build/src/addon.node"));
//   // if (!af.__extended) {     ext(af);     af.__extended = true; }
//   return ml;
// };
