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
  static MatlabMxArray *Create(napi_env env, napi_value jsthis);

  static napi_value Create(napi_env env, napi_callback_info info);

  static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);

  /**
   * \brief Constructor
   * 
   * Create a new MATLAB mxArray wrapper if none exists and returns the pointer
   * to the engine session as the node.js external data object. Multiple 
   * sessions of MATLAB could be opened by using different \ref id.
   * 
   * \param[in] session id number to support multiple MATLAB sessions
   */
  explicit MatlabMxArray(napi_env env, napi_value jsthis);

  /**
   * \brief Destrctor
   * 
   * Release the assigned MATLAB session
   */
  ~MatlabMxArray();


  bool setMxArray(mxArray *array); // will be responsible to destroy array
  const mxArray *getMxArray();     //

  napi_value getData(napi_env env); // Get mxArray content as a JavaScript data type
  napi_status setData(napi_env env, napi_value data); // Set mxArray content from JavaScript object
  
private:
  mxArray *array_; // data array
  std::map<void *, napi_ref> arraybuffers_;

  napi_env env_;
  napi_ref wrapper_;

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

  napi_value numeric_to_ext_value(napi_env env, const bool get_imag);
  template <typename data_type, typename MxGetFun>
  napi_value numeric_to_ext_typedarray(napi_env env, const napi_typedarray_type type, MxGetFun mxGet);

  napi_value to_value(napi_env env, const mxArray *array); // worker for GetData()
  template <typename data_type, typename MxGetFun>
  napi_value to_typedarray(napi_env env, const napi_typedarray_type type, const mxArray *array, MxGetFun mxGet); // logical scalar (or array with index arg)
  template <typename data_type>
  napi_value from_numeric(napi_env env, const mxArray *array, const napi_typedarray_type type); // logical scalar (or array with index arg)
  napi_value from_chars(napi_env env, const mxArray *array);                                    // char string
  napi_value from_logicals(napi_env env, const mxArray *array);                                 // numeric vector
  napi_value from_cell(napi_env env, const mxArray *array);                                     // for a struct
  napi_value from_struct(napi_env env, const mxArray *array, int index = -1);                   // for a cell

  mxArray *from_value(napi_env env, const napi_value value);  // worker for GetData()
  mxArray *from_object(napi_env env, const napi_value value); // for struct

  mxArray *from_typedarray(napi_env, const napi_value value);  // numeric vector
  mxArray *from_arraybuffer(napi_env, const napi_value value); // numeric vector
  mxArray *from_buffer(napi_env, const napi_value value);      // numeric vector
  mxArray *from_dataview(napi_env, const napi_value value);    // numeric vector
  mxArray *from_array(napi_env env, const napi_value value);   // for cell

  static napi_value getData(napi_env env, napi_callback_info info); // Get mxArray content as a JavaScript data type
  static napi_value setData(napi_env env, napi_callback_info info); // Set mxArray content from JavaScript object

  static napi_value getNumericDataByRef(napi_env env, napi_callback_info info); // Get mxArray content as a JavaScript data type

  static napi_value isDouble(napi_env env, napi_callback_info info);
  static napi_value isSingle(napi_env env, napi_callback_info info);
  static napi_value isComplex(napi_env env, napi_callback_info info);
  static napi_value isNumeric(napi_env env, napi_callback_info info);
  static napi_value isInt32(napi_env env, napi_callback_info info);
  static napi_value isUint32(napi_env env, napi_callback_info info);
  static napi_value isInt16(napi_env env, napi_callback_info info);
  static napi_value isUint16(napi_env env, napi_callback_info info);
  static napi_value isInt8(napi_env env, napi_callback_info info);
  static napi_value isUint8(napi_env env, napi_callback_info info);
  static napi_value isChar(napi_env env, napi_callback_info info);
  static napi_value isLogical(napi_env env, napi_callback_info info);
  static napi_value isInt64(napi_env env, napi_callback_info info);
  static napi_value isUint64(napi_env env, napi_callback_info info);
  static napi_value isEmpty(napi_env env, napi_callback_info info);
  static napi_value isScalar(napi_env env, napi_callback_info info);
  static napi_value isStruct(napi_env env, napi_callback_info info);
  static napi_value isCell(napi_env env, napi_callback_info info);

  static napi_value getNumberOfDimensions(napi_env env, napi_callback_info info);
  static napi_value getElementSize(napi_env env, napi_callback_info info);
  static napi_value getDimensions(napi_env env, napi_callback_info info);
  static napi_value getNumberOfElements(napi_env env, napi_callback_info info);
  static napi_value getM(napi_env env, napi_callback_info info);
  static napi_value getN(napi_env env, napi_callback_info info);
  static napi_value getNumberOfFields(napi_env env, napi_callback_info info);

  static napi_value setDimensions(napi_env env, napi_callback_info info);
  // static napi_value calcSingleSubscript(napi_env env, napi_callback_info info);
  // static napi_value setM(napi_env env, napi_callback_info info);
  // static napi_value setN(napi_env env, napi_callback_info info);

  // static napi_value getField(napi_env env, napi_callback_info info);
  // static napi_value setField(napi_env env, napi_callback_info info);
  // static napi_value addField(napi_env env, napi_callback_info info);
  // static napi_value removeField(napi_env env, napi_callback_info info);

  // static napi_value getCell(napi_env env, napi_callback_info info);
  // static napi_value setCell(napi_env env, napi_callback_info info);

  // static napi_value isClass(napi_env env, napi_callback_info info);
  // static napi_value getClassName(napi_env env, napi_callback_info info);
  // static napi_value setClassName(napi_env env, napi_callback_info info);
  // static napi_value getProperty(napi_env env, napi_callback_info info);
  // static napi_value setProperty(napi_env env, napi_callback_info info);

  // static napi_value isSparse(napi_env env, napi_callback_info info);
  // static napi_value getNzmax(napi_env env, napi_callback_info info);
  // static napi_value setNzmax(napi_env env, napi_callback_info info);
  // static napi_value getIr(napi_env env, napi_callback_info info);
  // static napi_value setIr(napi_env env, napi_callback_info info);
  // static napi_value getJc(napi_env env, napi_callback_info info);
  // static napi_value setJc(napi_env env, napi_callback_info info);
};
