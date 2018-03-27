#pragma once

#include <node_api.h>
#include <string>
#include <vector>
#include <utility>

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
  void *data(nullptr);
  napi_status status = napi_get_cb_info(env, info, &argc, argv.data(), &jsthis, &data);
  assert(status == napi_ok);
  if (argc < nargmin)
  {
    napi_throw_type_error(env, "", "Invalid number of arguments.");
    argv.clear();
  }
  else
    argv.resize(argc); // resize to only contain given arguments

  // grab the class instance if possible. Otherwise, obj=null
  T *obj(nullptr);
  napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));

  return NapiCBInfo<T>{jsthis, argv, obj, data};
}

/**
 * \brief convert node.js string value to utf8-encoded std::string
 */
inline std::string napi_get_value_string_utf8(napi_env env, napi_value value)
{
  napi_status status;

  // coarse the given node.js object to string type
  status = napi_coerce_to_string(env, value, &value);
  assert(status == napi_ok);

  // get the string length from string property
  napi_value val;
  status = napi_get_named_property(env, value, "length", &val);
  assert(status == napi_ok);

  uint32_t len_u32;
  status = napi_get_value_uint32(env, val, &len_u32);
  assert(status == napi_ok);

  // create string buffer
  size_t length;
  std::string expr(len_u32 + 1, 0);
  status = napi_get_value_string_utf8(env, value, expr.data(), expr.size(), &length);
  assert(status == napi_ok);

  return expr;
}
