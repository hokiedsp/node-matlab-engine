// defines an native addon node.js object to interface with MATLAB engine
//    .evaluate(expr)
//    .setVariable(value)
//    .putVariable
//    .getVariable
//    .setOutputBuffer

#pragma once

#include <mex.h>
#include <node_api.h>

#include <string>

class MatlabEngine;

/**
 * \brief Node.js class object wrapping MATLAB mxArray
 * 
 * 
 */
class MatlabMxArray
{
public:
  static napi_ref constructor;

  static napi_value Init(napi_env env, napi_value exports);

  static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);

  void setMxArray(const MatlabEngine &engine, const std::string &name, mxArray *array); // will be responsible to destroy array
  const mxArray *getMxArray(const MatlabEngine &engine, const std::string &name); // 

private:
  mxArray *array;   // data array
  
  /**
 * \brief Constructor
 * 
 * Create a new MATLAB mxArray wrapper if none exists and returns the pointer
 * to the engine session as the node.js external data object. Multiple 
 * sessions of MATLAB could be opened by using different \ref id.
 * 
 * \param[in] session id number to support multiple MATLAB sessions
 */
  explicit MatlabMxArray();

   /**
 * \brief Destrctor
 * 
 * Release the assigned MATLAB session
 */
 ~MatlabMxArray();

/**
 * \brief Create new MatlabMxArray object
 * 
 * javascript signature
 *    new matlab.MxArray()
 * 
 * \param[in] env  The environment that the API is invoked under.
 * \param[in] info The callback info passed into the callback function.
 * \returns napi_value representing the JavaScript object returned, which in 
 *          this case is the constructed object.
 */
  static napi_value create(napi_env env, napi_callback_info info);

  /**
 * \brief Evluates MATLAB expression
 */
  static napi_value ToBoolean(napi_env env, napi_callback_info info);    // logical scalar (or array with index arg)
  static napi_value ToNumber(napi_env env, napi_callback_info info);     // numeric scalar (or array with index arg)
  static napi_value ToString(napi_env env, napi_callback_info info);     // char string
  static napi_value ToTypedArray(napi_env env, napi_callback_info info); // numeric vector
  static napi_value ToObject(napi_env env, napi_callback_info info);     // for a simple struct
  static napi_value ToArray(napi_env env, napi_callback_info info);      // for a simple cell
  
  static napi_value FromBoolean(napi_env env, napi_callback_info info);    // logical scalar
  static napi_value FromNumber(napi_env env, napi_callback_info info);     // numeric scalar
  static napi_value FromString(napi_env env, napi_callback_info info);     // char string
  static napi_value FromTypedArray(napi_env env, napi_callback_info info); // numeric vector
  static napi_value FromObject(napi_env env, napi_callback_info info);     // for struct
  static napi_value FromArray(napi_env env, napi_callback_info info);      // for cell

  napi_value to_boolean(const mxArray *array, const int index = 0); // logical scalar (or array with index arg)
  napi_value to_number(const mxArray *array, const int index = 0);     // numeric scalar (or array with index arg)
  napi_value to_string(const mxArray *array);     // char string
  napi_value to_typed_array(const mxArray *array); // numeric vector
  napi_value to_object(const mxArray *array);     // for a struct
  napi_value to_array(const mxArray *array);      // for a cell

  mxArray *from_boolean(napi_env env, napi_callback_info info);    // logical scalar
  mxArray *from_number(napi_env env, napi_callback_info info);     // numeric scalar
  mxArray *from_string(napi_env env, napi_callback_info info);     // char string
  mxArray *from_typed_array(napi_env env, napi_callback_info info); // numeric vector
  mxArray *from_object(napi_env env, napi_callback_info info);     // for struct
  mxArray *from_array(napi_env env, napi_callback_info info);      // for cell
};
