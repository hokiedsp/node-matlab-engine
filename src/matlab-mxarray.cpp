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
      DECLARE_NAPI_METHOD("toBoolean", MatlabMxArray::ToBoolean),           // logical scalar (or array with index arg)
      DECLARE_NAPI_METHOD("toNumber", MatlabMxArray::ToNumber),             // numeric scalar (or array with index arg)
      DECLARE_NAPI_METHOD("toString", MatlabMxArray::ToString),             // char string
      DECLARE_NAPI_METHOD("toTypedArray", MatlabMxArray::ToTypedArray),     // numeric vector
      DECLARE_NAPI_METHOD("toObject", MatlabMxArray::ToObject),             // for a simple struct
      DECLARE_NAPI_METHOD("toArray", MatlabMxArray::ToArray),               // for a simple cell
      DECLARE_NAPI_METHOD("fromBoolean", MatlabMxArray::FromBoolean),       // logical scalar
      DECLARE_NAPI_METHOD("fromNumber", MatlabMxArray::FromNumber),         // numeric scalar
      DECLARE_NAPI_METHOD("fromString", MatlabMxArray::FromString),         // char string
      DECLARE_NAPI_METHOD("fromTypedArray", MatlabMxArray::FromTypedArray), // numeric vector
      DECLARE_NAPI_METHOD("fromObject", MatlabMxArray::FromObject),         // for struct
      DECLARE_NAPI_METHOD("fromArray", MatlabMxArray::FromArray)            // for cell
  };

  //
  napi_value cons;
  status = napi_define_class(env, "MatlabMxArray", NAPI_AUTO_LENGTH, MatlabMxArray::create,
                             nullptr, 6, properties, &cons);
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

  // check how the function is invoked
  napi_value target;
  status = napi_get_new_target(env, info, &target);
  assert(status == napi_ok);
  if (target != nullptr) // Invoked as constructor: `new MatlabMxArray(...)`
  {
    // retrieve details about the call
    size_t argc = 0; // one optional argument: <double> id
    napi_value jsthis;
    status = napi_get_cb_info(env, info, &argc, nullptr, &jsthis, nullptr);
    assert(status == napi_ok && argc != 0); // no argument allowed

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
    status = napi_new_instance(env, cons, argc, argv, &instance);
    assert(status == napi_ok);

    return instance;
  }
}

#define GetMxArrayAndIndex()                                                \
  \
{                                                                        \
    MatlabMxArray *obj;                                                     \
    size_t argc = 1;                                                        \
    napi_value args[1];                                                     \
    napi_value jsthis;                                                      \
    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);    \
    assert(status == napi_ok && argc < 2);                                  \
    status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));     \
    assert(status == napi_ok);                                              \
    if (argc > 0)                                                           \
    {                                                                       \
      double indexf;                                                        \
      status = napi_get_value_double(env, args[0], &indexf);                \
      assert(status == napi_ok && (indexf == double(index = (int)indexf))); \
    }                                                                       \
    array = obj->array;                                                     \
  \
}

napi_value MatlabMxArray::ToBoolean(napi_env env, napi_callback_info info) // logical scalar (or array with index arg)
{
  napi_status status;
  int index;
  mxArray *array;

  GetMxArrayAndIndex(array, index);
  return MatlabMxArray::to_boolean(array, index);
}

napi_value MatlabMxArray::ToNumber(napi_env env, napi_callback_info info) // numeric scalar (or array with index arg)
{
  napi_status status;
  int index;
  mxArray *array;

  GetMxArrayAndIndex(array, index);
  return MatlabMxArray::to_number(array, index);
}

napi_value MatlabMxArray::to_boolean(const mxArray *array, const int index = 0) // logical scalar (or array with index arg)
{
  napi_value rval(nullptr);
  if (mxIsEmpty(array))
  {
    status = napi_get_null(env, &rval);
    assert(status==napi_ok);
  }
  else if (mxIsLogical(array))
  {
    if (mxGetNumberOfElements(array) <= index)
    {
      status = napi_throw_error(env, code, "Index out of range.");
      assert(status == napi_ok);
    }
    bool *tfs = mxGetLogicals(array);
    status= napi_get_boolean(env, tfs[index], &rval);
  }
  else
  {
    status = napi_throw_error(env, code, "MxArray is not a boolean type.");
    assert(status==napi_ok);
  }
  return rval;
}

napi_value MatlabMxArray::to_number(const mxArray *array, const int index = 0) // numeric scalar (or array with index arg)
{
  napi_value rval(nullptr);
  if (mxIsEmpty(array))
  {
    status=  napi_get_null(env, &rval);
    assert(status==napi_ok);
  }
  else if (mxIsDouble(array))
  {
    if (mxGetNumberOfElements(array) <= index)
    {
      status = napi_throw_error(env, code, "Index out of range.");
      assert(status == napi_ok);
    }
    double *values = mxGetPr(array);
    status = napi_create_double(napi_env env, values[index], &rval);
    assert(status == napi_ok);
  }
  else if(mxIsNumeric(array))
  {
    status = napi_throw_error(env, code, "MxArray currently does not support non-double numeric Matlab data.");
    assert(status==napi_ok);
  }
  else
  {
    status = napi_throw_error(env, code, "MxArray is not a numeric type.");
    assert(status==napi_ok);
  }
  return rval;
}

#define GetMxArray()                                                        \
  \
{                                                                        \
    MatlabMxArray *obj;                                                     \
    size_t argc = 0;                                                        \
    napi_value jsthis;                                                      \
    status = napi_get_cb_info(env, info, &argc, nullptr, &jsthis, nullptr); \
    assert(status == napi_ok && argc == 0);                                 \
    status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));     \
    assert(status == napi_ok);                                              \
  \
}

napi_value MatlabMxArray::ToString(napi_env env, napi_callback_info info) // char string
{
  napi_status status;
  mxArray *array;

  GetMxArray(array);
  return MatlabMxArray::to_string(array, index);
}

napi_value MatlabMxArray::to_string(const mxArray *array) // char string
{
  napi_value rval(nullptr);
  if (mxIsEmpty(array))
  {
    status = napi_get_null(env, &rval);
    assert(status==napi_ok);
  }
  else if (mxIsChar(array))
  {
    const char16_t* str = mxGetChars(array);
    status = napi_create_string_utf16(env,  str,NAPI_AUTO_LENGTH, &rval);
  }
  else
  {
    status = napi_throw_error(env, code, "MxArray is not a char type.");
    assert(status==napi_ok);
  }
  return rval;
}


napi_value MatlabMxArray::ToTypedArray(napi_env env, napi_callback_info info) // numeric vector
{
}
napi_value MatlabMxArray::ToObject(napi_env env, napi_callback_info info) // for a simple struct
{
}
napi_value MatlabMxArray::ToArray(napi_env env, napi_callback_info info) // for a simple cell
{
}

napi_value MatlabMxArray::FromBoolean(napi_env env, napi_callback_info info) // logical scalar
{
}
napi_value MatlabMxArray::FromNumber(napi_env env, napi_callback_info info) // numeric scalar
{
}
napi_value MatlabMxArray::FromString(napi_env env, napi_callback_info info) // char string
{
}
napi_value MatlabMxArray::FromTypedArray(napi_env env, napi_callback_info info) // numeric vector
{
}
napi_value MatlabMxArray::FromObject(napi_env env, napi_callback_info info) // for struct
{
}
napi_value MatlabMxArray::FromArray(napi_env env, napi_callback_info info) // for cell
{
}

napi_value to_typed_array(const mxArray *array) // numeric vector
{
}
napi_value to_object(const mxArray *array) // for a struct
{
}
napi_value to_array(const mxArray *array) // for a cell
{
}

mxArray *from_boolean(napi_env env, napi_callback_info info) // logical scalar
{
}
mxArray *from_number(napi_env env, napi_callback_info info) // numeric scalar
{
}
mxArray *from_string(napi_env env, napi_callback_info info) // char string
{
}
mxArray *from_typed_array(napi_env env, napi_callback_info info) // numeric vector
{
}
mxArray *from_object(napi_env env, napi_callback_info info) // for struct
{
}
mxArray *from_array(napi_env env, napi_callback_info info) // for cell
{
}
