#include "matlab-mxarray.h"
#include "napi_utils.h"

#include <memory>
#include <vector>

typedef std::unique_ptr<mxArray, decltype(mxDestroyArray) *> managedMxArray;

napi_ref MatlabMxArray::constructor;

MatlabMxArray::MatlabMxArray() : env_(nullptr), wrapper_(nullptr), array_(nullptr) {}

MatlabMxArray::~MatlabMxArray()
{
  if (array_)
    mxDestroyArray(array_);
}

void MatlabMxArray::Destructor(napi_env env, void *nativeObject, void * /*finalize_hint*/)
{
  reinterpret_cast<MatlabMxArray *>(nativeObject)->~MatlabMxArray();
}

// macro to create napi_property_descriptor initializer list
#define DECLARE_NAPI_METHOD(name, func)     \
  {                                         \
    name, 0, func, 0, 0, 0, napi_default, 0 \
  }

template <typename T, int N>
char (&dim_helper(T (&)[N]))[N];
#define dim(x) (sizeof(dim_helper(x)))

napi_value MatlabMxArray::Init(napi_env env, napi_value exports)
{
  napi_status status;

  // define all the class (static) member functions as node.js array
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_METHOD("getData", MatlabMxArray::getData),
      DECLARE_NAPI_METHOD("setData", MatlabMxArray::setData),
      DECLARE_NAPI_METHOD("getNumericDataByRef", MatlabMxArray::getNumericDataByRef),
      DECLARE_NAPI_METHOD("isDouble", MatlabMxArray::isDouble),
      DECLARE_NAPI_METHOD("isSingle", MatlabMxArray::isSingle),
      DECLARE_NAPI_METHOD("isComplex", MatlabMxArray::isComplex),
      DECLARE_NAPI_METHOD("isNumeric", MatlabMxArray::isNumeric),
      DECLARE_NAPI_METHOD("isInt32", MatlabMxArray::isInt32),
      DECLARE_NAPI_METHOD("isUint32", MatlabMxArray::isUint32),
      DECLARE_NAPI_METHOD("isInt16", MatlabMxArray::isInt16),
      DECLARE_NAPI_METHOD("isUint16", MatlabMxArray::isUint16),
      DECLARE_NAPI_METHOD("isInt8", MatlabMxArray::isInt8),
      DECLARE_NAPI_METHOD("isUint8", MatlabMxArray::isUint8),
      DECLARE_NAPI_METHOD("isChar", MatlabMxArray::isChar),
      DECLARE_NAPI_METHOD("isLogical", MatlabMxArray::isLogical),
      DECLARE_NAPI_METHOD("isInt64", MatlabMxArray::isInt64),
      DECLARE_NAPI_METHOD("isUint64", MatlabMxArray::isUint64),
      DECLARE_NAPI_METHOD("isEmpty", MatlabMxArray::isEmpty),
      DECLARE_NAPI_METHOD("isScalar", MatlabMxArray::isScalar),
      DECLARE_NAPI_METHOD("isStruct", MatlabMxArray::isStruct),
      DECLARE_NAPI_METHOD("isCell", MatlabMxArray::isCell),
      DECLARE_NAPI_METHOD("getNumberOfDimensions", MatlabMxArray::getNumberOfDimensions),
      DECLARE_NAPI_METHOD("getElementSize", MatlabMxArray::getElementSize),
      DECLARE_NAPI_METHOD("getDimensions", MatlabMxArray::getDimensions),
      DECLARE_NAPI_METHOD("setDimensions", MatlabMxArray::setDimensions),
      DECLARE_NAPI_METHOD("getNumberOfElements", MatlabMxArray::getNumberOfElements),
      DECLARE_NAPI_METHOD("getM", MatlabMxArray::getM),
      DECLARE_NAPI_METHOD("getN", MatlabMxArray::getN),
      DECLARE_NAPI_METHOD("getNumberOfFields", MatlabMxArray::getNumberOfFields)};

  //
  napi_value cons;
  status = napi_define_class(env, "MatlabMxArray", NAPI_AUTO_LENGTH, MatlabMxArray::create,
                             nullptr, dim(properties), properties, &cons);
  assert(status == napi_ok);

  status = napi_create_reference(env, cons, 1, &MatlabMxArray::constructor);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "MxArray", cons);
  assert(status == napi_ok);
  return exports;
}

// create new instance of the class
napi_value MatlabMxArray::Create(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 1);
  if (prhs.argv.size() > 1)
    return nullptr;

  // check how the function is invoked
  napi_value target;
  status = napi_get_new_target(env, info, &target);
  assert(status == napi_ok);
  if (target != nullptr) // Invoked as constructor: `new MatlabMxArray(...)`
  {
    // instantiate new class object
    MatlabMxArray *obj = new MatlabMxArray();
    obj->env_ = env; // setremember object's environment

    // Wraps the new native instance in a JavaScript object.
    status = napi_wrap(env, prhs.jsthis, reinterpret_cast<void *>(obj),
                       MatlabMxArray::Destructor, nullptr, &obj->wrapper_);
    assert(status == napi_ok);

    // if source napi_value given, populate
    if (prhs.argv.size())
      obj->from_value(env, prhs.argv[0]);

    return prhs.jsthis;
  }
  else // Invoked as plain function `MatlabMxArray(...)`, turn into construct call.
  {
    napi_value cons;
    status = napi_get_reference_value(env, constructor, &cons);
    assert(status == napi_ok);

    // call this function again but invoked as constructor
    napi_value instance;
    status = napi_new_instance(env, cons, prhs.argv.size(), prhs.argv.data(), &instance);
    assert(status == napi_ok);

    return instance;
  }
}

const mxArray *MatlabMxArray::getMxArray()
{
  return array_;
}

// data = mx_array.getData() Get mxArray content as a JavaScript data type
napi_value MatlabMxArray::getData(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  return prhs.obj ? prhs.obj->to_value(env, prhs.obj->array_) : nullptr;
}

bool MatlabMxArray::setMxArray(mxArray *array) // will be responsible to destroy array
{
  // mxArray cannot have anything referencing to it
  if (arraybuffers_.size())
    return true;

  // destroy previous array
  if (array_)
    mxDestroyArray(array_);

  // assign the new array
  array_ = array;

  return false;
}

// data = mx_array.setData(value) Set mxArray content from JavaScript object
napi_value MatlabMxArray::setData(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 1, 1);
  if (!prhs.obj)
    return nullptr;

  // mxArray cannot have anything referencing to it
  if (prhs.obj->arraybuffers_.size())
  {
    status = napi_throw_error(env, "", "Cannot assign new data to the MxArray instance as it is currently referenced by at least one Node.js value.");
    assert(status == napi_ok);
    return nullptr;
  };

  // convert the given Node.js value to mxArray
  mxArray *new_array = prhs.obj->from_value(env, prhs.argv[0]);

  // destroy previous array
  if (prhs.obj->array_)
    mxDestroyArray(prhs.obj->array_);

  // assign the new array
  prhs.obj->array_ = new_array;

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief data = mx_array.getNumericDataByRef(get_imag) 
 * 
 * Get mxArray content as external-data typed array so that the mxArray data can be
 * modified directly from JavaScript
 */
napi_value MatlabMxArray::getNumericDataByRef(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 1);
  if (!prhs.obj)
    return nullptr;

  // retrieve details about the call
  bool get_imag = prhs.argv.size();
  if (get_imag)
  {
    // coarse the given node.js object to string type
    status = napi_get_value_bool(env, prhs.argv[0], &get_imag);
    if (status != napi_ok)
    {
      napi_throw_type_error(env, "", "get_imag must be coercable to Boolean type.");
      return nullptr;
    }
  }

  return prhs.obj->numeric_to_ext_value(env, get_imag);
}

napi_value MatlabMxArray::numeric_to_ext_value(napi_env env, const bool get_imag)
{
  napi_status status;
  napi_value rval(nullptr);
  if (mxIsEmpty(array_))
  {
    status = napi_get_null(env, &rval);
    assert(status == napi_ok);
  }
  else
  {
    auto mxGet = mxGetData;
    if (get_imag)
    {
      mxGet = mxGetImagData;
      if (!mxIsComplex(array_)) // if imaginary part does not exist, allocate
        mxSetImagData(array_, mxCalloc(mxGetNumberOfElements(array_), mxGetElementSize(array_)));
    }

    switch (mxGetClassID(array_))
    {
    case mxDOUBLE_CLASS:
      rval = numeric_to_ext_typedarray<double, decltype(mxGetData) *>(env, napi_float64_array, mxGet);
      break;
    case mxSINGLE_CLASS:
      rval = numeric_to_ext_typedarray<float, decltype(mxGetData) *>(env, napi_float32_array, mxGet);
      break;
    case mxINT8_CLASS:
      rval = numeric_to_ext_typedarray<int8_t, decltype(mxGetData) *>(env, napi_int8_array, mxGet);
      break;
    case mxUINT8_CLASS:
      rval = numeric_to_ext_typedarray<uint8_t, decltype(mxGetData) *>(env, napi_uint8_array, mxGet);
      break;
    case mxINT16_CLASS:
      rval = numeric_to_ext_typedarray<int16_t, decltype(mxGetData) *>(env, napi_int16_array, mxGet);
      break;
    case mxUINT16_CLASS:
      rval = numeric_to_ext_typedarray<uint16_t, decltype(mxGetData) *>(env, napi_uint16_array, mxGet);
      break;
    case mxINT32_CLASS:
      rval = numeric_to_ext_typedarray<int32_t, decltype(mxGetData) *>(env, napi_int32_array, mxGet);
      break;
    case mxUINT32_CLASS:
      rval = numeric_to_ext_typedarray<uint32_t, decltype(mxGetData) *>(env, napi_uint32_array, mxGet);
      break;
    case mxINT64_CLASS:
    case mxUINT64_CLASS:
    default:
      napi_throw_error(env, "", "Only numeric MATLAB classes are supported, except for INT64 and UINT64 as Node.js does not.");
    }
  }
  return rval;
}

template <typename data_type, typename MxGetFun>
napi_value MatlabMxArray::numeric_to_ext_typedarray(napi_env env, const napi_typedarray_type type, MxGetFun mxGet) // logical scalar (or array with index arg)
{
  napi_status status;
  napi_value value, arraybuffer;
  void *data = mxGet(array_);
  auto it = arraybuffers_.find(data); // locate existing reference to data
  if (it == arraybuffers_.end())      // create first reference
  {
    status = napi_create_external_arraybuffer(
        env, mxGet(array_), mxGetNumberOfElements(array_) * sizeof(data_type),
        [](napi_env env, void *array_data, void *obj_data) {
          MatlabMxArray *obj = reinterpret_cast<MatlabMxArray *>(obj_data);
          // finalize callback: unreference, if no other references, delete reference
          auto it = obj->arraybuffers_.find(array_data);
          assert(it != obj->arraybuffers_.end());

          uint32_t count;
          napi_status status = napi_reference_unref(env, it->second, &count);
          assert(status == napi_ok);
          if (!count)
          {
            status = napi_delete_reference(env, it->second);
            obj->arraybuffers_.erase(it);
          }
        },
        this, &arraybuffer);
    assert(status == napi_ok);
    napi_ref array_ref;
    status = napi_create_reference(env, arraybuffer, 1, &array_ref);
    assert(status == napi_ok);
    arraybuffers_.emplace(std::make_pair(reinterpret_cast<void *>(mxGet(array_)), array_ref));
  }
  else // external arraybuffer object already exists, reference it
  {
    uint32_t count;
    status = napi_reference_ref(env, it->second, &count);
    assert(status == napi_ok);
    status = napi_get_reference_value(env, it->second, &arraybuffer);
    assert(status == napi_ok);
  }

  // create typed array
  status = napi_create_typedarray(env, type, mxGetNumberOfElements(array_), arraybuffer, 0, &value);
  return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

napi_value MatlabMxArray::to_value(napi_env env, const mxArray *array) // worker for GetData()
{
  napi_status status;
  napi_value rval(nullptr);
  if (mxIsEmpty(array))
  {
    status = napi_get_null(env, &rval);
    assert(status == napi_ok);
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
      napi_throw_error(env, "", "Unknown or unsupported mxArray type.");
    }
  }
  return rval;
}

template <typename data_type, typename MxGetFun>
napi_value MatlabMxArray::to_typedarray(napi_env env, const napi_typedarray_type type, const mxArray *array, MxGetFun mxGet) // logical scalar (or array with index arg)
{
  napi_status status;
  napi_value value, arraybuffer;

  data_type *data;
  size_t nelem = mxGetNumberOfElements(array);
  status = napi_create_arraybuffer(env, nelem * sizeof(data_type), &reinterpret_cast<void *>(data), &arraybuffer);
  assert(status == napi_ok);

  // copy data
  std::copy_n(reinterpret_cast<data_type *>(mxGet(array)), nelem, data);

  // create typed array
  status = napi_create_typedarray(env, type, mxGetNumberOfElements(array), arraybuffer, 0, &value);
  return value;
}

template <typename data_type>
napi_value MatlabMxArray::from_numeric(napi_env env, const mxArray *array, const napi_typedarray_type type) // logical scalar (or array with index arg)
{
  napi_value rval(nullptr);
  napi_status status;
  if (mxIsComplex(array)) // complex data, create object ot hold both real & imaginary parts
  {
    status = napi_create_object(env, &rval);
    assert(status == napi_ok);

    status = napi_set_named_property(env, rval, "re",
                                     to_typedarray<data_type,
                                                   decltype(mxGetData) *>(env, type, array, mxGetData));
    assert(status == napi_ok);

    status = napi_set_named_property(env, rval, "im",
                                     to_typedarray<data_type,
                                                   decltype(mxGetImagData) *>(env, type, array, mxGetImagData));
    assert(status == napi_ok);
  }
  else if (mxIsDouble(array)) // return a typedarray object pointing directly at mxArray data
    status = napi_create_double(env, mxGetScalar(array), &rval);
  else
    rval = to_typedarray<data_type, decltype(mxGetData) *>(env, type, array, mxGetData);

  return rval;
}

napi_value MatlabMxArray::from_chars(napi_env env, const mxArray *array) // char string
{
  napi_value rval;
  napi_status status = napi_create_string_utf16(env, mxGetChars(array), NAPI_AUTO_LENGTH, &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::from_logicals(napi_env env, const mxArray *array)
{
  napi_status status;
  napi_value rval(nullptr);
  if (mxIsScalar(array))
  {
    status = napi_get_boolean(env, mxGetLogicals(array)[0], &rval);
    assert(status == napi_ok);
  }
  else
  {
    status = napi_throw_error(env, "", "Non-scalar logical variable is not currently supported.");
    // napi_value script;
    // status = napi_create_string_utf8(env, "require('bitset')", NAPI_AUTO_LENGTH, &script);
    // assert(status == napi_ok);

    // napi_value bitset_module;
    // status = napi_run_script(env, script, &bitset_module);
  }
  return rval;
}

napi_value MatlabMxArray::from_cell(napi_env env, const mxArray *array) // for a struct
{
  napi_status status;
  napi_value rval;

  uint32_t nelem = (uint32_t)mxGetNumberOfElements(array);
  status = napi_create_array_with_length(env, nelem, &rval);
  assert(status == napi_ok);
  for (uint32_t i = 0; i < nelem; ++i) // recursively call from_struct() to populate each element
  {
    status = napi_set_element(env, rval, i, to_value(env, mxGetCell(array, i)));
    assert(status == napi_ok);
  }
  return rval;
}

napi_value MatlabMxArray::from_struct(napi_env env, const mxArray *array, int index) // for a cell
{
  napi_status status;
  napi_value rval;

  if (index < 0 && mxIsScalar(array))
    index = 0;

  if (index >= 0)
  {
    status = napi_create_object(env, &rval);
    assert(status == napi_ok);

    int nfields = mxGetNumberOfFields(array);
    for (int n = 0; n < nfields; ++n)
    {
      status = napi_set_named_property(env, rval, mxGetFieldNameByNumber(array, n),
                                       to_value(env, mxGetFieldByNumber(array, index, n)));
      assert(status == napi_ok);
    }
  }
  else // struct of array -> array of object
  {
    int nelem = (int)mxGetNumberOfElements(array);
    status = napi_create_array_with_length(env, nelem, &rval);
    assert(status == napi_ok);
    for (int i = 0; i < nelem; ++i) // recursively call from_struct() to populate each element
    {
      status = napi_set_element(env, rval, i, from_struct(env, array, i));
      assert(status == napi_ok);
    }
  }
  return rval;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

mxArray *MatlabMxArray::from_value(napi_env env, const napi_value value) // worker for GetData()
{
  napi_status status;
  napi_valuetype type;
  status = napi_typeof(env, value, &type);
  assert(status == napi_ok);

  switch (type)
  {
  case napi_null:
    return mxCreateDoubleMatrix(0, 0, mxREAL);
  case napi_boolean:
  {
    bool bool_val;
    status = napi_get_value_bool(env, value, &bool_val);
    assert(status == napi_ok);
    return mxCreateLogicalScalar(bool_val);
  }
  case napi_number:
  {
    double dbl_val;
    status = napi_get_value_double(env, value, &dbl_val);
    assert(status == napi_ok);
    return mxCreateDoubleScalar(dbl_val);
  }
  case napi_string:
  {
    std::string str_val = napi_get_value_string_utf8(env, value);
    return mxCreateString(str_val.c_str());
  }
  case napi_object:
    return from_object(env, value);
  case napi_symbol:
  case napi_undefined:
  case napi_function:
  case napi_external:
  default:
    napi_throw_error(env, "", "Unsupported value type.");
    return nullptr;
  }
}

mxArray *MatlabMxArray::from_object(napi_env env, const napi_value value)
{
  bool is_type;
  napi_status status = napi_is_array(env, value, &is_type);
  assert(status == napi_ok);
  if (is_type) // array -> cell
    return from_array(env, value);

  status = napi_is_typedarray(env, value, &is_type);
  assert(status == napi_ok);
  if (is_type) // typedarray -> numeric vector
    return from_typedarray(env, value);

  status = napi_is_arraybuffer(env, value, &is_type);
  assert(status == napi_ok);
  if (is_type) // typedarray -> uint8 vector
    return from_arraybuffer(env, value);

  status = napi_is_buffer(env, value, &is_type);
  assert(status == napi_ok);
  if (is_type) // buffer -> uint8 vector
    return from_buffer(env, value);

  status = napi_is_dataview(env, value, &is_type);
  assert(status == napi_ok);
  if (is_type) // dataview -> uint8 vector
    return from_dataview(env, value);

  status = napi_is_error(env, value, &is_type);
  assert(status == napi_ok);
  if (is_type) // error -> throw it back
  {
    status = napi_throw(env, value);
    return nullptr;
  }

  // plain object or a class constructor/instance -> value-only struct

  // get array of property names
  napi_value pnamevalues;
  status = napi_get_property_names(env, value, &pnamevalues);
  assert(status == napi_ok);

  // get number of properties
  uint32_t nfields;
  status = napi_get_array_length(env, pnamevalues, &nfields);
  assert(status == napi_ok);

  // create field name vector
  std::vector<std::string> pnames;
  std::vector<const char *> fnames;
  pnames.reserve(nfields);
  fnames.reserve(nfields);
  for (uint32_t i = 0; i < nfields; ++i)
  {
    napi_handle_scope scope;
    status = napi_open_handle_scope(env, &scope);
    assert(status == napi_ok);

    napi_value pname;
    status = napi_get_element(env, pnamevalues, i, &pname);
    pnames.push_back(napi_get_value_string_utf8(env, pname));
    fnames.push_back(pnames.back().c_str());

    status = napi_close_handle_scope(env, scope);
    assert(status == napi_ok);
  }

  // create mxArray
  managedMxArray array(mxCreateStructMatrix(1, 1, nfields, fnames.data()), mxDestroyArray);

  // populate fields
  for (uint32_t i = 0; i < nfields; ++i)
  {
    napi_handle_scope scope;
    status = napi_open_handle_scope(env, &scope);
    assert(status == napi_ok);

    napi_value pval;
    status = napi_get_named_property(env, pnamevalues, fnames[i], &pval);
    assert(status == napi_ok);

    mxSetField(array.get(), 0, fnames[i], from_value(env, pval));

    status = napi_close_handle_scope(env, scope);
    assert(status == napi_ok);
  }

  mxArray *rval = array.release();
  return rval;
}

mxArray *MatlabMxArray::from_array(napi_env env, const napi_value value) // for cell
{
  uint32_t length;
  napi_status status = napi_get_array_length(env, value, &length);
  assert(status == napi_ok);

  // create mxArray
  managedMxArray array(mxCreateCellMatrix(length, 1), mxDestroyArray);

  // populate it
  for (uint32_t i = 0; i < length; ++i)
  {
    napi_handle_scope scope;
    status = napi_open_handle_scope(env, &scope);
    assert(status == napi_ok);

    napi_value pval;
    status = napi_get_element(env, value, i, &pval);
    assert(status == napi_ok);

    mxSetCell(array.get(), i, from_value(env, pval));

    status = napi_close_handle_scope(env, scope);
    assert(status == napi_ok);
  }

  mxArray *rval = array.release();
  return rval;
}

mxArray *MatlabMxArray::from_typedarray(napi_env env, const napi_value value) // numeric vector
{
  napi_status status;
  napi_typedarray_type type;
  size_t length;
  void *data;
  napi_value arraybuffer;
  size_t byte_offset;

  status = napi_get_typedarray_info(env, value, &type, &length, &data, &arraybuffer, &byte_offset);
  assert(status == napi_ok);

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
    status = napi_throw_error(env, "", "Unsupported typedarray type.");
  }
  return rval;
}

mxArray *MatlabMxArray::from_arraybuffer(napi_env env, const napi_value value) // numeric vector
{
  napi_status status;
  void *data;
  size_t byte_length;
  status = napi_get_arraybuffer_info(env, value, &data, &byte_length);
  assert(status == napi_ok);

  mxArray *rval = mxCreateNumericMatrix(byte_length, 1, mxUINT8_CLASS, mxREAL);
  std::copy_n((uint8_t *)data, byte_length, (uint8_t *)mxGetData(rval));
  return rval;
}

mxArray *MatlabMxArray::from_buffer(napi_env env, const napi_value value) // numeric vector
{
  void *data;
  size_t length;
  napi_status status = napi_get_buffer_info(env, value, &data, &length);
  assert(status == napi_ok);
  mxArray *rval = mxCreateNumericMatrix(length, 1, mxUINT8_CLASS, mxREAL);
  std::copy_n((uint8_t *)data, length, (uint8_t *)mxGetData(rval));
  return rval;
}

mxArray *MatlabMxArray::from_dataview(napi_env env, const napi_value value) // numeric vector
{
  size_t byte_length;
  void *data;
  napi_value arraybuffer;
  size_t byte_offset;
  napi_status status = napi_get_dataview_info(env, value, &byte_length, &data, &arraybuffer, &byte_offset);
  assert(status == napi_ok);
  mxArray *rval = mxCreateNumericMatrix(byte_length, 1, mxUINT8_CLASS, mxREAL);
  std::copy_n((uint8_t *)data, byte_length, (uint8_t *)mxGetData(rval));
  return rval;
}

///////////////////////////////////////////////////////////////////////////////

napi_value MatlabMxArray::isDouble(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsDouble(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isSingle(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsSingle(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isComplex(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsComplex(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isNumeric(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsNumeric(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isInt32(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsInt32(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isUint32(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsUint32(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isInt16(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsInt16(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isUint16(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsUint16(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isInt8(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsInt8(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isUint8(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsUint8(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isChar(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsChar(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isLogical(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsLogical(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isInt64(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsInt64(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isUint64(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsUint64(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isEmpty(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsEmpty(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isScalar(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsScalar(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isStruct(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsStruct(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::isCell(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_get_boolean(env, mxIsCell(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}

napi_value MatlabMxArray::getNumberOfDimensions(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_create_double(env, (double)mxGetNumberOfDimensions(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}
napi_value MatlabMxArray::getElementSize(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_create_double(env, (double)mxGetElementSize(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}
napi_value MatlabMxArray::getNumberOfElements(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_create_double(env, (double)mxGetNumberOfElements(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}
napi_value MatlabMxArray::getM(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_create_double(env, (double)mxGetM(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}
napi_value MatlabMxArray::getN(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_create_double(env, (double)mxGetN(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}
napi_value MatlabMxArray::getNumberOfFields(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  napi_value rval;
  napi_status status = napi_create_double(env, (double)mxGetNumberOfFields(prhs.obj->array_), &rval);
  assert(status == napi_ok);
  return rval;
}
napi_value MatlabMxArray::getDimensions(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 0, 0);
  if (!prhs.obj)
    return nullptr;

  size_t ndims = mxGetNumberOfDimensions(prhs.obj->array_);
  const size_t *dims = mxGetDimensions(prhs.obj->array_);

  napi_value rval;
  napi_status status = napi_create_array_with_length(env, ndims, &rval);
  assert(status == napi_ok);
  for (int n = 0; n < ndims; ++n)
  {
    napi_handle_scope scope;
    status = napi_open_handle_scope(env, &scope);
    assert(status == napi_ok);

    napi_value elem;
    status = napi_create_double(env, (double)dims[n], &elem);
    assert(status == napi_ok);

    napi_set_element(env, rval, n, elem);
    assert(status == napi_ok);

    status = napi_close_handle_scope(env, scope);
    assert(status == napi_ok);
  }
  return rval;
}

napi_value MatlabMxArray::setDimensions(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 1, 1);
  if (!prhs.obj)
    return nullptr;

  uint32_t ndims;
  
  // get new dimensions
  status = napi_get_array_length(env, prhs.argv[0], &ndims);
  if (status != napi_ok)
  {
    napi_throw_type_error(env, "", "Dimensions must be given as an array.");
    return nullptr;
  }

  std::vector<mwSize> dims(ndims, 1);
  mwSize nelems = 1;
  for (int n = 0; n < dims.size(); ++n)
  {
    napi_handle_scope scope;
    status = napi_open_handle_scope(env, &scope);
    assert(status == napi_ok);

    napi_value elem;
    status = napi_get_element(env, prhs.argv[0], n, &elem);

    uint32_t val;
    status = napi_get_value_uint32(env,elem, &val);
    if (status!=napi_ok)
    {
      napi_throw_type_error(env, "", "Dimensions must be non-negative integers.");
      return nullptr;
    }

    dims[n] = val;
    nelems *= val;

    status = napi_close_handle_scope(env, scope);
    assert(status == napi_ok);
  }

  // validate size
  if (nelems!=mxGetNumberOfElements(prhs.obj->array_))
  {
    napi_throw_type_error(env, "", "Dimensions must yield the number of elements of the array.");
    return nullptr;
  }

  // set it
  mxSetDimensions(prhs.obj->array_, dims.data(), dims.size());

  return nullptr;
}

// napi_value MatlabMxArray::calcSingleSubscript(napi_env env, napi_callback_info info)
// {
//   napi_status status;

//   // retrieve the input arguments
//   auto prhs = napi_get_cb_info<MatlabMxArray>(env, info, 1, 1);
//   if (!prhs.obj)
//     return nullptr;

//   mwIndex index = mxCalcSingleSubscript(prhs.obj->array_, mwSize nsubs, mwIndex *subs);
// }
