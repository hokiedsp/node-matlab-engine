#pragma once

#include <node_api.h>
#include <string>
#include <vector>
#include <utility>
#include <stdexcept>

template <typename T, int N>
char (&dim_helper(T (&)[N]))[N];
#define dim(x) (sizeof(dim_helper(x)))

template <class T>
struct NapiCBInfo
{
  napi_value jsthis;
  std::vector<napi_value> argv;
  T *obj;
  void *data;
};

/**
  * \brief Retrieve JavaScript callback arguments
  *
  */
template <class T>
NapiCBInfo<T> napi_get_cb_info(napi_env env, napi_callback_info info, size_t nargmin, size_t nargmax)
{
  napi_value jsthis(nullptr);
  size_t argc = nargmax;
  std::vector<napi_value> argv(nargmax, nullptr);
  T *obj(nullptr);
  void *data(nullptr);

  if (napi_get_cb_info(env, info, &argc, argv.data(), &jsthis, &data) != napi_ok)
    throw std::runtime_error("Failed to call napi_get_cb_info() in napi_get_cb_info().");

  if (argc < nargmin)
    throw std::runtime_error("Invalid number of arguments.");

  argv.resize(argc); // resize to only contain given arguments

  // grab the class instance if possible. Otherwise, obj=null
  napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));

  return NapiCBInfo<T>{jsthis, argv, obj, data};
}

/**
 * \brief convert node.js string value to utf8-encoded std::string
 */
inline std::string napi_get_value_string_utf8(napi_env env, napi_value value)
{
  // coarse the given node.js object to string type
  if (napi_coerce_to_string(env, value, &value) != napi_ok)
    throw std::runtime_error("Failed to execute napi_coerce_to_string()");

  // get the string length from string property
  napi_value val;
  if (napi_get_named_property(env, value, "length", &val) != napi_ok)
    throw std::runtime_error("Failed to execute napi_get_named_property()");

  uint32_t len_u32;
  if (napi_get_value_uint32(env, val, &len_u32) != napi_ok)
    throw std::runtime_error("Failed to execute napi_get_value_uint32()");

  // create string buffer
  size_t length;
  std::string expr(len_u32 + 1, 0);
  if (napi_get_value_string_utf8(env, value, expr.data(), expr.size(), &length) != napi_ok)
    throw std::runtime_error("Failed to execute napi_get_value_string_utf8()");

  return expr;
}

inline bool value2bool(napi_env env, napi_value value, bool coerce = true)
{
  bool rval;
  napi_status status = napi_get_value_bool(env, value, &rval);
  if (coerce && status == napi_boolean_expected) // non-bool object, try to coerce
  {
    napi_value coerced;
    if (napi_coerce_to_bool(env, value, &coerced) == napi_ok)
      status = napi_get_value_bool(env, value, &rval);
  }
  if (status != napi_ok)
    throw std::runtime_error("Failed to convert a JavaScript object to bool.");
  return rval;
}

inline uint32_t value2uint32(napi_env env, napi_value value, bool coerce = true)
{
  uint32_t rval;
  napi_status status = napi_get_value_uint32(env, value, &rval);
  if (coerce && status == napi_number_expected) // non-bool object, try to coerce
  {
    napi_value coerced;
    if (napi_coerce_to_number(env, value, &coerced) == napi_ok)
      status = napi_get_value_uint32(env, value, &rval);
  }
  if (status != napi_ok)
    throw std::runtime_error("Failed to convert a JavaScript object to uint32.");
  return rval;
}

inline double value2double(napi_env env, napi_value value, bool coerce = true)
{
  double rval;
  napi_status status = napi_get_value_double(env, value, &rval);
  if (coerce && status == napi_number_expected) // non-bool object, try to coerce
  {
    napi_value coerced;
    if (napi_coerce_to_number(env, value, &coerced) == napi_ok)
      status = napi_get_value_double(env, value, &rval);
  }
  if (status != napi_ok)
    throw std::runtime_error("Failed to convert a JavaScript object to double.");
  return rval;
}
