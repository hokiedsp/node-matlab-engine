#include "matlab-mxarray.h"

MatlabMxArray::MatlabMxArray() : env_(nullptr), wrapper_(nullptr), array(nullptr) {}

MatlabMxArray::~MatlabMxArray()
{
  if (array)
    mxDestroyArray(array);
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

napi_value MatlabMxArray::Init(napi_env env, napi_value exports)
{
  napi_status status;

  // define all the class (static) member functions as node.js array
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_METHOD("getData", MatlabMxArray::GetData), // logical scalar (or array with index arg)
      DECLARE_NAPI_METHOD("setData", MatlabMxArray::SetData)  // numeric scalar (or array with index arg)
  };

  //
  napi_value cons;
  status = napi_define_class(env, "MatlabMxArray", NAPI_AUTO_LENGTH, MatlabMxArray::create,
                             nullptr, 2, properties, &cons);
  assert(status == napi_ok);

  status = napi_create_reference(env, cons, 1, &MatlabMxArray::constructor);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "MatlabMxArray", cons);
  assert(status == napi_ok);
  return exports;
}

// create new instance of the class
napi_value MatlabMxArray::create(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve details about the call
  size_t argc = 0; // one optional argument: <double> id
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, nullptr, &jsthis, nullptr);
  assert(status == napi_ok && argc != 0); // no argument allowed

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
    status = napi_wrap(env, jsthis, reinterpret_cast<void *>(obj),
                       MatlabMxArray::Destructor, nullptr, &obj->wrapper_);
    assert(status == napi_ok);

    return jsthis;
  }
  else // Invoked as plain function `MatlabMxArray(...)`, turn into construct call.
  {
    napi_value cons;
    status = napi_get_reference_value(env, constructor, &cons);
    assert(status == napi_ok);

    // call this function again but invoked as constructor
    napi_value instance;
    status = napi_new_instance(env, cons, 0, nullptr, &instance);
    assert(status == napi_ok);

    return instance;
  }
}


napi_value MatlabMxArray::to_value(napi_env env, const mxArray *array) // worker for GetData()
{
  napi_status status;
  napi_value rval(nullptr);
  if (mxIsEmpty(array))
  {
    status = napi_get_null(env,&rval);
    assert(status==napi_ok);
  }
  else
  {
    switch (mxGetClassID(array))
    {
    case mxCELL_CLASS:
      rval = from_cell(env,array);
      break;
    case mxSTRUCT_CLASS:
      rval = from_struct(env,array);
      break;
    case mxLOGICAL_CLASS:
      rval = from_logicals(env,array);
      break;
    case mxCHAR_CLASS:
      rval = from_chars(env,array);
      break;
    case mxDOUBLE_CLASS:
      rval = from_numeric<double>(env,array,napi_float64_array);
      break;
    case mxSINGLE_CLASS:
      rval = from_numeric<float>(env,array,napi_float32_array);
      break;
    case mxINT8_CLASS:
      rval = from_numeric<int8_t>(env,array,napi_int8_array);
      break;
    case mxUINT8_CLASS:
      rval = from_numeric<uint8_t>(env,array,napi_uint8_array);
      break;
    case mxINT16_CLASS:
      rval = from_numeric<int16_t>(env,array,napi_int16_array);
      break;
    case mxUINT16_CLASS:
      rval = from_numeric<uint16_t>(env,array,napi_uint16_array);
      break;
    case mxINT32_CLASS:
      rval = from_numeric<int32_t>(env,array,napi_int32_array);
      break;
    case mxUINT32_CLASS:
      rval = from_numeric<uint32_t>(env,array,napi_uint32_array);
      break;
    case mxINT64_CLASS:
    case mxUINT64_CLASS:
    case mxVOID_CLASS:
    case mxFUNCTION_CLASS:
    case mxUNKNOWN_CLASS:
    default:
      napi_throw_error(env,"","Unknown or unsupported mxArray type.");
    }
  } 
  return rval;
}

template <typename data_type, typename MxGetFun>
napi_value MatlabMxArray::to_typedarray(napi_env env, const napi_typedarray_type type, const mxArray *array, MxGetFun mxGet) // logical scalar (or array with index arg)
{
  napi_status status;
  napi_value value, arraybuffer;
  void *data = mxGet(array);
  auto it = arraybuffers.find(data); // locate existing reference to data
  if (it == arraybuffers.end())      // create first reference
  {
    status = napi_create_external_arraybuffer(
        env, mxGet(array), mxGetNumberOfElements(array) * sizeof(data_type),
        [](napi_env env, void *array_data, void *obj_data) {
          MatlabMxArray *obj = reinterpret_cast<MatlabMxArray *>(obj_data);
          // finalize callback: unreference, if no other references, delete reference
          auto it = obj->arraybuffers.find(array_data);
          assert(it != obj->arraybuffers.end());

          uint32_t count;
          napi_status status = napi_reference_unref(env, it->second, &count);
          assert(status == napi_ok);
          if (!count)
          {
            status = napi_delete_reference(env, it->second);
            obj->arraybuffers.erase(it);
          }
        },
        this, &arraybuffer);
    assert(status == napi_ok);
    napi_ref array_ref;
    status = napi_create_reference(env, arraybuffer, 1, &array_ref);
    assert(status == napi_ok);
    arraybuffers.emplace(std::make_pair(reinterpret_cast<void *>(mxGet(array)), array_ref));
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
                                     to_typedarray<data_type, decltype(mxGetData) *>(env, type, array, mxGetData));
    assert(status == napi_ok);

    status = napi_set_named_property(env, rval, "im",
                                     to_typedarray<data_type, decltype(mxGetImagData) *>(env, type, array, mxGetImagData));
    assert(status == napi_ok);
  }
  else // return a typedarray object pointing directly at mxArray data
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

// mxArray *from_boolean(napi_env env, napi_callback_info info);    // logical scalar
// mxArray *from_number(napi_env env, napi_callback_info info);     // numeric scalar
// mxArray *from_string(napi_env env, napi_callback_info info);     // char string
// mxArray *from_typed_array(napi_env env, napi_callback_info info); // numeric vector
// mxArray *from_object(napi_env env, napi_callback_info info);     // for struct
// mxArray *from_array(napi_env env, napi_callback_info info);      // for cell
