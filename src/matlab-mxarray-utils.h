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
#include <stdexcept>

typedef std::unique_ptr<mxArray, decltype(mxDestroyArray) *> managedMxArray;

// helper function prototypes
template <typename data_type, typename MxGetFun>
napi_value to_typedarray(napi_env env, const napi_typedarray_type type, const mxArray *array, MxGetFun mxGet); // logical scalar (or array with index arg)
template <typename data_type>
napi_value from_numeric(napi_env env, const mxArray *array, const napi_typedarray_type type); // logical scalar (or array with index arg)
napi_value from_chars(napi_env env, const mxArray *array);                                    // char string
napi_value from_logicals(napi_env env, const mxArray *array);
napi_value from_cell(napi_env env, const mxArray *array);                   // for a struct
napi_value from_struct(napi_env env, const mxArray *array, int index = -1); // for a cell
mxArray *from_object(napi_env env, const napi_value value);
mxArray *from_array(napi_env env, const napi_value value);       // for cell
mxArray *from_typedarray(napi_env env, const napi_value value);  // numeric vector
mxArray *from_arraybuffer(napi_env env, const napi_value value); // numeric vector
mxArray *from_buffer(napi_env env, const napi_value value);      // numeric vector
mxArray *from_dataview(napi_env env, const napi_value value);    // numeric vector

/**
 * \brief   Convert form Matlab mxArray to N-API value
 * 
 * \param[in] env N-API context
 * \param[in] array Matlab mxArray opaque object
 * \returns N-API value containing a copy of val
 */
inline napi_value mxArrayToNapiValue(napi_env env, const mxArray *array)
{
  napi_value rval(nullptr);
  if (mxIsEmpty(array))
  {
    if (napi_get_null(env, &rval) != napi_ok)
      throw std::runtime_error("Failed to create null JavaScript object");
  }
  else
  {
    switch (mxGetClassID(array))
    {
    case mxCELL_CLASS:
      rval = from_cell(env, array);
      break;
    case mxSTRUCT_CLASS:
      rval = from_struct(env, array);
      break;
    case mxLOGICAL_CLASS:
      rval = from_logicals(env, array);
      break;
    case mxCHAR_CLASS:
      rval = from_chars(env, array);
      break;
    case mxDOUBLE_CLASS:
      rval = from_numeric<double>(env, array, napi_float64_array);
      break;
    case mxSINGLE_CLASS:
      rval = from_numeric<float>(env, array, napi_float32_array);
      break;
    case mxINT8_CLASS:
      rval = from_numeric<int8_t>(env, array, napi_int8_array);
      break;
    case mxUINT8_CLASS:
      rval = from_numeric<uint8_t>(env, array, napi_uint8_array);
      break;
    case mxINT16_CLASS:
      rval = from_numeric<int16_t>(env, array, napi_int16_array);
      break;
    case mxUINT16_CLASS:
      rval = from_numeric<uint16_t>(env, array, napi_uint16_array);
      break;
    case mxINT32_CLASS:
      rval = from_numeric<int32_t>(env, array, napi_int32_array);
      break;
    case mxUINT32_CLASS:
      rval = from_numeric<uint32_t>(env, array, napi_uint32_array);
      break;
    case mxINT64_CLASS:
    case mxUINT64_CLASS:
    case mxVOID_CLASS:
    case mxFUNCTION_CLASS:
    case mxUNKNOWN_CLASS:
    default:
      throw std::runtime_error("Unknown or unsupported mxArray type.");
    }
  }
  return rval;
}

inline mxArray *napiValueToMxArray(napi_env env, const napi_value value)
{
  napi_valuetype type;
  if (napi_typeof(env, value, &type) != napi_ok)
    throw std::runtime_error("Failed to get the type of the JavaScript value.");

  switch (type)
  {
  case napi_null:
    return mxCreateDoubleMatrix(0, 0, mxREAL);
  case napi_boolean:
  {
    bool bool_val;
    if (napi_get_value_bool(env, value, &bool_val) != napi_ok)
      throw std::runtime_error("Failed to get boolean JavaScript value.");
    return mxCreateLogicalScalar(bool_val);
  }
  case napi_number:
  {
    double dbl_val;
    if (napi_get_value_double(env, value, &dbl_val) != napi_ok)
      throw std::runtime_error("Failed to get numeric JavaScript value.");
    return mxCreateDoubleScalar(dbl_val);
  }
  case napi_string:
  {
    std::string str_val = napi_get_value_string_utf8(env, value);
    return mxCreateString(str_val.c_str());
  }
  case napi_object: // array or object or class?
    return from_object(env, value);
  case napi_symbol:
  case napi_undefined:
  case napi_function:
  case napi_external:
  default:
    throw std::runtime_error("napiValueToMxArray: Unsupported value type.");
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///   Matlab mxArray to N-API value helper functions

template <typename data_type, typename MxGetFun>
napi_value to_typedarray(napi_env env, const napi_typedarray_type type, const mxArray *array, MxGetFun mxGet) // logical scalar (or array with index arg)
{
  napi_value value, arraybuffer;

  data_type *data;
  size_t nelem = mxGetNumberOfElements(array);
  if (napi_create_arraybuffer(env, nelem * sizeof(data_type), &reinterpret_cast<void *>(data), &arraybuffer) != napi_ok)
    throw std::runtime_error("Failed to create JavaScript Array Buffer.");

  // copy data
  std::copy_n(reinterpret_cast<data_type *>(mxGet(array)), nelem, data);

  // create typed array
  if (napi_create_typedarray(env, type, mxGetNumberOfElements(array), arraybuffer, 0, &value) != napi_ok)
    throw std::runtime_error("Failed to create typed array.");
  return value;
}

template <typename data_type>
napi_value from_numeric(napi_env env, const mxArray *array, const napi_typedarray_type type) // logical scalar (or array with index arg)
{
  napi_value rval(nullptr);
  if (mxIsComplex(array)) // complex data, create object ot hold both real & imaginary parts
  {
    if (napi_create_object(env, &rval) != napi_ok)
      throw std::runtime_error("Failed to create JavaScript object to hold complex number.");

    if (napi_set_named_property(env, rval, "re",
                                to_typedarray<data_type,
                                              decltype(mxGetData) *>(env, type, array, mxGetData)) != napi_ok)
      throw std::runtime_error("Failed to set re property.");

    if (napi_set_named_property(env, rval, "im",
                                to_typedarray<data_type,
                                              decltype(mxGetImagData) *>(env, type, array, mxGetImagData)) != napi_ok)
      throw std::runtime_error("Failed to create im property");
  }
  else if (mxIsDouble(array)) // return a typedarray object pointing directly at mxArray data
  {
    if (napi_create_double(env, mxGetScalar(array), &rval) != napi_ok)
      throw std::runtime_error("Failed to create a double value");
  }
  else
  {
    rval = to_typedarray<data_type, decltype(mxGetData) *>(env, type, array, mxGetData);
  }

  return rval;
}

inline napi_value from_chars(napi_env env, const mxArray *array) // char string
{
  napi_value rval;
  if (napi_create_string_utf16(env, mxGetChars(array), NAPI_AUTO_LENGTH, &rval) != napi_ok)
    throw std::runtime_error("Failed to create JavaScript string.");
  return rval;
}

inline napi_value from_logicals(napi_env env, const mxArray *array)
{
  napi_value rval(nullptr);
  if (mxIsScalar(array))
  {
    if (napi_get_boolean(env, mxGetLogicals(array)[0], &rval) != napi_ok)
      throw std::runtime_error("Failed to create a boolean value.");
  }
  else
  {
    throw std::runtime_error("Non-scalar logical variable is not currently supported.");
  }
  return rval;
}

inline napi_value from_cell(napi_env env, const mxArray *array) // for a struct
{
  napi_value rval;

  uint32_t nelem = (uint32_t)mxGetNumberOfElements(array);
  if (napi_create_array_with_length(env, nelem, &rval) != napi_ok)
    throw std::runtime_error("Failed to create JavaScript array.");
  for (uint32_t i = 0; i < nelem; ++i) // recursively call from_struct() to populate each element
  {
    if (napi_set_element(env, rval, i, mxArrayToNapiValue(env, mxGetCell(array, i))) != napi_ok)
      throw std::runtime_error("Failed to set an JavaScript array element.");
  }
  return rval;
}

inline napi_value from_struct(napi_env env, const mxArray *array, int index) // for a cell
{
  napi_value rval;

  if (index < 0 && mxIsScalar(array))
    index = 0;

  if (index >= 0)
  {
    if (napi_create_object(env, &rval) != napi_ok)
      throw std::runtime_error("Failed to create JavaScript object.");

    int nfields = mxGetNumberOfFields(array);
    for (int n = 0; n < nfields; ++n)
    {
      if (napi_set_named_property(env, rval, mxGetFieldNameByNumber(array, n),
                                  mxArrayToNapiValue(env, mxGetFieldByNumber(array, index, n))) != napi_ok)
        throw std::runtime_error("Failed to set JavaScript object property.");
    }
  }
  else // struct of array -> array of object
  {
    int nelem = (int)mxGetNumberOfElements(array);
    if (napi_create_array_with_length(env, nelem, &rval) != napi_ok)
      throw std::runtime_error("Failed to create a JavaScript array.");
    for (int i = 0; i < nelem; ++i) // recursively call from_struct() to populate each element
    {
      if (napi_set_element(env, rval, i, from_struct(env, array, i)) != napi_ok)
        throw std::runtime_error("Failed to set JavaScript array element.");
    }
  }
  return rval;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///   N-API value to Matlab mxArray helper functions

inline mxArray *from_object(napi_env env, const napi_value value)
{
  bool is_type;
  if (napi_is_array(env, value, &is_type) == napi_ok && is_type)
    return from_array(env, value);

  if (napi_is_typedarray(env, value, &is_type) == napi_ok && is_type)
    return from_typedarray(env, value);

  if (napi_is_arraybuffer(env, value, &is_type) == napi_ok && is_type)
    return from_arraybuffer(env, value);

  if (napi_is_buffer(env, value, &is_type) == napi_ok && is_type)
    return from_buffer(env, value);

  if (napi_is_dataview(env, value, &is_type) == napi_ok && is_type)
    return from_dataview(env, value);

  if (napi_is_error(env, value, &is_type) == napi_ok && is_type)
    throw std::runtime_error("Cannot convert JavaScript error object to MATLAB mxArray.");

  // plain object or a class constructor/instance -> value-only struct

  // get array of property names
  napi_value pnamevalues;
  if (napi_get_property_names(env, value, &pnamevalues) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_property_names()");

  // get number of properties
  uint32_t nfields;
  if (napi_get_array_length(env, pnamevalues, &nfields) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_array_length()");

  // create field name vector
  std::vector<std::string> pnames;
  std::vector<const char *> fnames;
  pnames.reserve(nfields);
  fnames.reserve(nfields);
  for (uint32_t i = 0; i < nfields; ++i)
  {
    napi_handle_scope scope;
    if (napi_open_handle_scope(env, &scope) != napi_ok)
      throw std::runtime_error("Failed to run napi_open_handle_scope()");

    napi_value pname;
    if (napi_get_element(env, pnamevalues, i, &pname) != napi_ok)
      throw std::runtime_error("Failed to run napi_get_element()");
    pnames.push_back(napi_get_value_string_utf8(env, pname));
    fnames.push_back(pnames.back().c_str());

    if (napi_close_handle_scope(env, scope) != napi_ok)
      throw std::runtime_error("Failed to run napi_close_handle_scope()");
  }

  // create mxArray
  managedMxArray array(mxCreateStructMatrix(1, 1, nfields, fnames.data()), mxDestroyArray);

  // populate fields
  for (uint32_t i = 0; i < nfields; ++i)
  {
    napi_handle_scope scope;
    if (napi_open_handle_scope(env, &scope) != napi_ok)
      throw std::runtime_error("Failed to run napi_open_handle_scope()");

    napi_value pval;
    if (napi_get_named_property(env, value, fnames[i], &pval) != napi_ok)
      throw std::runtime_error("Failed to run napi_get_named_property()");

    mxSetField(array.get(), 0, fnames[i], napiValueToMxArray(env, pval));

    if (napi_close_handle_scope(env, scope) != napi_ok)
      throw std::runtime_error("Failed to run napi_close_handle_scope()");
  }

  mxArray *rval = array.release();
  return rval;
}

inline mxArray *from_array(napi_env env, const napi_value value) // for cell
{
  uint32_t length;
  if (napi_get_array_length(env, value, &length) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_array_length()");

  // create mxArray
  managedMxArray array(mxCreateCellMatrix(length, 1), mxDestroyArray);

  // populate it
  for (uint32_t i = 0; i < length; ++i)
  {
    napi_handle_scope scope;
    if (napi_open_handle_scope(env, &scope) != napi_ok)
      throw std::runtime_error("Failed to run napi_open_handle_scope()");

    napi_value pval;
    if (napi_get_element(env, value, i, &pval) != napi_ok)
      throw std::runtime_error("Failed to run napi_get_element()");

    mxSetCell(array.get(), i, napiValueToMxArray(env, pval));

    if (napi_close_handle_scope(env, scope) != napi_ok)
      throw std::runtime_error("Failed to run napi_close_handle_scope()");
  }

  mxArray *rval = array.release();
  return rval;
}

inline mxArray *from_typedarray(napi_env env, const napi_value value) // numeric vector
{
  napi_typedarray_type type;
  size_t length;
  void *data;
  napi_value arraybuffer;
  size_t byte_offset;

  if (napi_get_typedarray_info(env, value, &type, &length, &data, &arraybuffer, &byte_offset) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_typedarray_info()");

  mxArray *rval(nullptr);

  switch (type)
  {
  case napi_int8_array:
    rval = mxCreateNumericMatrix(length, 1, mxINT8_CLASS, mxREAL);
    std::copy_n(((int8_t *)data) + byte_offset, length, (int8_t *)mxGetData(rval));
    break;
  case napi_uint8_array:
    rval = mxCreateNumericMatrix(length, 1, mxUINT8_CLASS, mxREAL);
    std::copy_n(((uint8_t *)data) + byte_offset, length, (uint8_t *)mxGetData(rval));
    break;
  case napi_int16_array:
    rval = mxCreateNumericMatrix(length, 1, mxINT16_CLASS, mxREAL);
    std::copy_n(((int16_t *)data) + byte_offset, length, (int16_t *)mxGetData(rval));
    break;
  case napi_uint16_array:
    rval = mxCreateNumericMatrix(length, 1, mxUINT16_CLASS, mxREAL);
    std::copy_n(((int16_t *)data) + byte_offset, length, (uint16_t *)mxGetData(rval));
    break;
  case napi_int32_array:
    rval = mxCreateNumericMatrix(length, 1, mxINT32_CLASS, mxREAL);
    std::copy_n(((int32_t *)data) + byte_offset, length, (int32_t *)mxGetData(rval));
    break;
  case napi_uint32_array:
    rval = mxCreateNumericMatrix(length, 1, mxUINT32_CLASS, mxREAL);
    std::copy_n(((uint32_t *)data) + byte_offset, length, (uint32_t *)mxGetData(rval));
    break;
  case napi_float32_array:
    rval = mxCreateNumericMatrix(length, 1, mxSINGLE_CLASS, mxREAL);
    std::copy_n(((float *)data) + byte_offset, length, (float *)mxGetData(rval));
    break;
  case napi_float64_array:
    rval = mxCreateNumericMatrix(length, 1, mxDOUBLE_CLASS, mxREAL);
    std::copy_n(((double *)data) + byte_offset, length, (double *)mxGetData(rval));
    break;
  case napi_uint8_clamped_array:
  default:
    throw std::runtime_error("Unsupported typedarray type.");
  }
  return rval;
}

inline mxArray *from_arraybuffer(napi_env env, const napi_value value) // numeric vector
{
  void *data;
  size_t byte_length;
  if (napi_get_arraybuffer_info(env, value, &data, &byte_length) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_arraybuffer_info()");

  mxArray *rval = mxCreateNumericMatrix(byte_length, 1, mxUINT8_CLASS, mxREAL);
  std::copy_n((uint8_t *)data, byte_length, (uint8_t *)mxGetData(rval));
  return rval;
}

inline mxArray *from_buffer(napi_env env, const napi_value value) // numeric vector
{
  void *data;
  size_t length;
  if (napi_get_buffer_info(env, value, &data, &length) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_buffer_info()");
  mxArray *rval = mxCreateNumericMatrix(length, 1, mxUINT8_CLASS, mxREAL);
  std::copy_n((uint8_t *)data, length, (uint8_t *)mxGetData(rval));
  return rval;
}

inline mxArray *from_dataview(napi_env env, const napi_value value) // numeric vector
{
  size_t byte_length;
  void *data;
  napi_value arraybuffer;
  size_t byte_offset;
  if (napi_get_dataview_info(env, value, &byte_length, &data, &arraybuffer, &byte_offset) != napi_ok)
    throw std::runtime_error("Failed to run napi_get_dataview_info()");
  mxArray *rval = mxCreateNumericMatrix(byte_length, 1, mxUINT8_CLASS, mxREAL);
  std::copy_n((uint8_t *)data, byte_length, (uint8_t *)mxGetData(rval));
  return rval;
}
