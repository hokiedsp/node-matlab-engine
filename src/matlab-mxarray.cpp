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

template <typename data_type, typename MxGetFun>
napi_value MatlabMxArray::to_typedarray(napi_env env, const napi_typedarray_type type, mxArray *array, MxGetFun mxGet) // logical scalar (or array with index arg)
{
  napi_value value;
  void *data = mxGet(array);
  auto it = arraybuffers.find(data); // locate existing reference to data
  if (it == arraybuffers.end())      // create first reference
  {
    napi_value a;
    status = napi_create_external_arraybuffer(
        env, mxGet(array), mxGetNumberOfElements(array) * sizeof(data_type),
        [](napi_env env, void *array_data, MatlabMxArray *obj) {
          // finalize callback: unreference, if no other references, delete reference
          auto it = obj->arraybuffers.find(array_data);
          assert(it != obj->arraybuffers.end());

          int count;
          napi_status status = napi_reference_unref(env, it->second, &count);
          assert(status == napi_ok);
          if (!count)
          {
            status = napi_delete_reference(env, it->second);
            obj->arraybuffers.erase(it);
          }
        },
        this, &value);
    assert(status == napi_ok);
    status = napi_create_reference(env, value, 1, &arraybuffer);
    assert(status == napi_ok);
  }
  else // external arraybuffer object already exists, reference it
  {
    int count;
    status = napi_reference_ref(env, arraybuffer, &count);
    assert(status == napi_ok);
    status = napi_get_reference_value(env, arraybuffer, &value);
    assert(status == napi_ok);
  }

  // create typed array
  status = napi_create_typedarray(env, type, mxGetNumberOfElements(array),
                                  arraybuffer, 0, &value);
  return value;
}

template <typename data_type>
napi_value MatlabMxArray::from_numeric(napi_env env, const mxArray *array, const napi_typedarray_type type) // logical scalar (or array with index arg)
{
  napi_value rval(nullptr);
  if (mxIsReal(array)) // return a typedarray object pointing directly at mxArray data
    rval = to_typedarray<data_type, decltype(mxGetData) *>(env, array, mxGetData, type);
  else // complex data, create object ot hold both real & imaginary parts
  {
    status = napi_create_object(env, &rval);
    assert(status == napi_ok);

    status = napi_set_named_property(env, rval, "re",
                                     to_typedarray<data_type, decltype(mxGetData) *>(env, array, mxGetData, type));
    assert(status == napi_ok);

    status = napi_set_named_property(env, rval, "im",
                                     to_typedarray<data_type, decltype(mxGetImagData) *>(env, array, mxGetImagData, type));
    assert(status == napi_ok);
  }

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

  size_t nelem = mxGetNumberOfElements(array);
  status = napi_create_array_with_length(env, nelem, &rval);
  assert(status == napi_ok);
  for (size_t i = 0; i < nelem; ++i) // recursively call from_struct() to populate each element
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
    size_t nelem = mxGetNumberOfElements(array);
    status = napi_create_array_with_length(env, nelem, &rval);
    assert(status == napi_ok);
    for (size_t i = 0; i < nelem; ++i) // recursively call from_struct() to populate each element
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
