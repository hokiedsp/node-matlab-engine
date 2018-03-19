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
#include <map>

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

  void setMxArray(mxArray *array); // will be responsible to destroy array
  const mxArray *getMxArray();     //

private:
  mxArray *array; // data array
  std::map<void *, napi_ref> arraybuffers;

  napi_env env_;
  napi_ref wrapper_;

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
  // GetNumberOfDimensions	//Number of dimensions in array
  // GetElementSize	//Number of bytes required to store each data element
  // GetDimensions	//Pointer to dimensions array
  // SetDimensions	/Modify number of dimensions and size of each dimension
  // GetNumberOfElements	//Number of elements in array
  // CalcSingleSubscript	//Offset from first element to desired element
  // GetM	//Number of rows in array
  // SetM	//Set number of rows in array
  // GetN	//Number of columns in array
  // SetN	//Set number of columns in array

  static napi_value getData(napi_env env, napi_callback_info info); // Get mxArray content as a JavaScript data type

  napi_value to_value(napi_env env, const mxArray *array); // worker for GetData()
  template <typename data_type, typename MxGetFun>
  napi_value to_typedarray(napi_env env, const napi_typedarray_type type, const mxArray *array, MxGetFun mxGet); // logical scalar (or array with index arg)
  template <typename data_type>
  napi_value from_numeric(napi_env env, const mxArray *array, const napi_typedarray_type type); // logical scalar (or array with index arg)
  napi_value from_chars(napi_env env, const mxArray *array);                                    // char string
  napi_value from_logicals(napi_env env, const mxArray *array);                                 // numeric vector
  napi_value from_cell(napi_env env, const mxArray *array);                                     // for a struct
  napi_value from_struct(napi_env env, const mxArray *array, int index = -1);                   // for a cell

  static napi_value setData(napi_env env, napi_callback_info info); // Set mxArray content from JavaScript object
  mxArray *from_value(napi_env env, const napi_value value);        // worker for GetData()
  mxArray *from_boolean(napi_env env, const napi_value value);      // logical scalar
  mxArray *from_number(napi_env env, const napi_value value);       // numeric scalar
  mxArray *from_string(napi_env env, const napi_value value);       // char string
  mxArray *from_typed_array(napi_env, const napi_value value);      // numeric vector
  mxArray *from_object(napi_env env, const napi_value value);       // for struct
  mxArray *from_array(napi_env env, const napi_value value);        // for cell
};
