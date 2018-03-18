#include "matlab-engine.h"
#include "matlab-mxarray.h"

// matlab-engine module
// - keeps a count of every "require('matlab-engine')" module
// - closes matlab
// node interface object:
//    .evaluate(expr)
//    .setVariable(value)
//    .putVariable
//    .getVariable
//    .setOutputBuffer

napi_value Init(napi_env env, napi_value exports)
{
  // define all the class (static) member functions as node.js array
  napi_property_descriptor desc[] = {
      {"Engine", nullptr, nullptr, nullptr, nullptr, MatlabEngine::Init(env, exports), napi_default, 0},
      {"MxArray", nullptr, nullptr, nullptr, nullptr, MatlabMxArray::Init(env, exports), napi_default, 0}};

  napi_status status = napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
  assert(status == napi_ok);

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
