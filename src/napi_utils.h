#pragma once

#include <node_api.h>
#include <string>

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

