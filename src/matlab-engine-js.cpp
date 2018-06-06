#include "matlab-engine-js.h"
#include "napi_utils.h"
#include "matlab-mxarray-utils.h"

#include <stdexcept>
#include <string>

#ifdef DEBUG
#include <fstream>
std::ofstream os("test.txt", std::fstream::out);
#endif

napi_ref MatlabEngineJS::constructor;

// macro to create napi_property_descriptor initializer list
#define DECLARE_NAPI_METHOD(name, func)     \
  {                                         \
    name, 0, func, 0, 0, 0, napi_default, 0 \
  }

napi_value MatlabEngineJS::Init(napi_env env, napi_value exports)
{
  // define all the class (static) member functions as node.js array
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_METHOD("open", MatlabEngineJS::Open),
      DECLARE_NAPI_METHOD("close", MatlabEngineJS::Close),
      // DECLARE_NAPI_METHOD("eval", MatlabEngineJS::Eval),
      DECLARE_NAPI_METHOD("evalSync", MatlabEngineJS::EvalSync),
      // DECLARE_NAPI_METHOD("feval", MatlabEngineJS::Feval),
      // DECLARE_NAPI_METHOD("fevalSync", MatlabEngineJS::FevalSync),
      DECLARE_NAPI_METHOD("getVariable", MatlabEngineJS::GetVariable),
      DECLARE_NAPI_METHOD("putVariable", MatlabEngineJS::PutVariable),
      {"isOpen", 0, 0, MatlabEngineJS::GetIsOpen, 0, 0, napi_default, nullptr},
      {"visible", 0, 0, MatlabEngineJS::GetVisible, MatlabEngineJS::SetVisible, 0, napi_writable, nullptr},
      {"buffer", 0, 0, MatlabEngineJS::GetBuffer, 0, 0, napi_default, nullptr},
      {"bufferEnabled", 0, 0, MatlabEngineJS::GetBufferEnabled, MatlabEngineJS::SetBufferEnabled, 0, napi_writable, nullptr},
      {"bufferSize", 0, 0, MatlabEngineJS::GetBufferSize, MatlabEngineJS::SetBufferSize, 0, napi_writable, nullptr}};

  //define NodeJS class
  napi_value cons;
  if (napi_define_class(env, "MatlabEngine", NAPI_AUTO_LENGTH, MatlabEngineJS::Create,
                        nullptr, dim(properties), properties, &cons) != napi_ok)
    napi_fatal_error("MatlabEngineJS::Init", NAPI_AUTO_LENGTH, "Failed to define MatlabEngine class.", NAPI_AUTO_LENGTH);

  if (napi_create_reference(env, cons, 1, &MatlabEngineJS::constructor) != napi_ok)
    napi_fatal_error("MatlabEngineJS::Init", NAPI_AUTO_LENGTH, "Failed to create MatlabEngine class reference.", NAPI_AUTO_LENGTH);

  if (napi_set_named_property(env, exports, "Engine", cons) != napi_ok)
    napi_fatal_error("MatlabEngineJS::Init", NAPI_AUTO_LENGTH, "Failed to add MatlabEngine class constructor to the exported object.", NAPI_AUTO_LENGTH);

  return exports;
}

// create new instance of the class
//   new Matlab.Engine([id],[options])
//      options <boolean> | <Object>
//         defer_to_open <boolean> Default: false
napi_value MatlabEngineJS::Create(napi_env env, napi_callback_info info)
{
#ifdef DEBUG
  os << "MatlabEngineJS::create" << std::endl;
#endif

  // retrieve details about the call
  auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 1);

  napi_value target;
  if (napi_get_new_target(env, info, &target) != napi_ok)
    napi_fatal_error("MatlabEngineJS::Create", NAPI_AUTO_LENGTH, "Failed to call napi_get_new_target().", NAPI_AUTO_LENGTH);

  if (target) // Invoked as constructor: `new MatlabEngine(...)`
  {
    // instantiate new class object
    MatlabEngineJS *obj;
    try
    {
      if (prhs.argv.empty())
        obj = new MatlabEngineJS(env, prhs.jsthis);
      else
        obj = new MatlabEngineJS(env, prhs.jsthis, prhs.argv[0]);
    }
    catch (...)
    {
      napi_throw_error(env, "", "Failed to instantiate MatlabEngine");
      return nullptr;
    }

    return prhs.jsthis;
  }
  else // Invoked as plain function `MatlabEngineJS(...)`, turn into construct call.
  {
    napi_value cons;
    if (napi_get_reference_value(env, constructor, &cons) != napi_ok)
      napi_fatal_error("MatlabEngineJS::Create", NAPI_AUTO_LENGTH, "Failed to call napi_get_reference_value().", NAPI_AUTO_LENGTH);

    // call this function again but invoked as constructor
    napi_value instance;
    if (napi_new_instance(env, cons, prhs.argv.size(), prhs.argv.data(), &instance) != napi_ok)
      return nullptr;

    return instance;
  }
}

void MatlabEngineJS::Destructor(napi_env env, void *nativeObject, void * /*finalize_hint*/)
{
#ifdef DEBUG
  os << "MatlabEngineJS::Destructor" << std::endl;
#endif

  MatlabEngineJS *obj = reinterpret_cast<MatlabEngineJS *>(nativeObject);

  // release the instance from node.js
  if (obj->wrapper_)
    napi_delete_reference(env, obj->wrapper_);

  // delete the object
  delete obj;
}

/**
 * \brief Close existing session and destroys the native MatlabEngineJS object
 * 
 */
napi_value MatlabEngineJS::Open(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->open();
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

/**
 * \brief Close existing session and destroys the native MatlabEngineJS object
 * 
 * session.close()
 */
napi_value MatlabEngineJS::Close(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->close();
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

napi_value MatlabEngineJS::GetIsOpen(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    return prhs.obj->is_open(env);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
    return nullptr;
  }
}

napi_value MatlabEngineJS::GetVisible(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    return prhs.obj->get_visible(env);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
    return nullptr;
  }
}

napi_value MatlabEngineJS::GetBuffer(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    return prhs.obj->get_buffer(env);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
    return nullptr;
  }
}

napi_value MatlabEngineJS::GetBufferEnabled(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    return prhs.obj->get_buffer_enabled(env);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
    return nullptr;
  }
}

napi_value MatlabEngineJS::GetBufferSize(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 0);
    if (!prhs.obj)
      return nullptr;

    return prhs.obj->get_buffer_size(env);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
    return nullptr;
  }
}

napi_value MatlabEngineJS::SetVisible(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 1);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->set_visible(env, prhs.argv[0]);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

napi_value MatlabEngineJS::SetBufferEnabled(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 1);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->set_buffer_enabled(env, prhs.argv[0]);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

napi_value MatlabEngineJS::SetBufferSize(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 0, 1);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->set_buffer_size(env, prhs.argv[0]);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

/**
 * \brief Synchronously evluates MATLAB expression
 * 
 * session.EvalSync(expr)
 * output = session.EvalSync(expr) - to retrieve output buffer
 * 
 */
napi_value MatlabEngineJS::EvalSync(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 1, 1);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->eval(env, prhs.argv[0]);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

/**
 * \brief Copy variable from MATLAB engine workspace
 * 
 * value = session.GetVariable(name)
 */
napi_value MatlabEngineJS::GetVariable(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 1, 1);
    if (!prhs.obj)
      return nullptr;

    return prhs.obj->get_variable(env, prhs.argv[0]);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
    return nullptr;
  }
}

/**
 * \brief Put variable into MATLAB engine workspace
 * session.PutVariable(name, value)
 */
napi_value MatlabEngineJS::PutVariable(napi_env env, napi_callback_info info)
{
  try
  { // retrieve the input arguments
    auto prhs = napi_get_cb_info<MatlabEngineJS>(env, info, 2, 2);
    if (!prhs.obj)
      return nullptr;

    prhs.obj->put_variable(env, prhs.argv[0], prhs.argv[1]);
  }
  catch (std::exception &e)
  {
    napi_throw_error(env, "", e.what());
  }
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

MatlabEngineJS::MatlabEngineJS(napi_env env, napi_value jsthis, napi_value opt_value)
    : eng_(256, true), wrapper_(nullptr)
{
#ifdef DEBUG
  os << "MatlabEngineJS::MatlabEngineJS" << std::endl;
#endif

  // parse inputs
  bool defer_open = false;
  if (opt_value)
  {
    try
    {
      // if boolean type, defer_open option directly given
      defer_open = value2bool(env, opt_value);
    }
    catch (...)
    {
      // else, look for defer_open property
      try
      {
        bool has_prop;
        if (napi_has_named_property(env, opt_value, "defer_to_open", &has_prop) == napi_ok && has_prop)
        {
          if (napi_get_named_property(env, opt_value, "defer_to_open", &opt_value) != napi_ok)
            throw;
          defer_open = value2bool(env, opt_value);
        }
      }
      catch (...)
      {
        throw std::runtime_error("Matlab Engine ID must be a double value.");
      }
    }
  }

  // Wraps the new native instance in a JavaScript object.
  napi_status status = napi_wrap(env, jsthis, this, MatlabEngineJS::Destructor, nullptr, &wrapper_);
  assert(status == napi_ok);

  // open MATLAB session if not deferred
  if (!defer_open)
    eng_.open();
}

void MatlabEngineJS::open()
{
  eng_.open();
}

/**
 * \brief Close existing session and destroys the native MatlabEngineJS object
 * 
 * \param[in] env  The environment that the API is invoked under.
 * \param[in] info The callback info passed into the callback function.
 * \returns napi_value representing the JavaScript object returned, which in 
 *          this case is the constructed object.
 */
void MatlabEngineJS::close()
{
  // remove the C++ object from Node wrapper object
  eng_.close();
}

napi_value MatlabEngineJS::is_open(napi_env env)
{
  napi_value rval = nullptr;
  if (napi_get_boolean(env, eng_.isopen(), &rval) != napi_ok)
    napi_throw_error(env, "", "napi_get_boolean() failed.");
  return rval;
}

napi_value MatlabEngineJS::get_visible(napi_env env)
{
  napi_value rval = nullptr;
  if (napi_get_boolean(env, eng_.getVisible(), &rval) != napi_ok)
    napi_throw_error(env, "", "napi_get_boolean() failed.");
  return rval;
}

napi_value MatlabEngineJS::get_buffer_enabled(napi_env env)
{
  napi_value rval = nullptr;
  if (napi_get_boolean(env, eng_.getBufferEnabled(), &rval) != napi_ok)
    napi_throw_error(env, "", "napi_get_boolean() failed.");
  return rval;
}

napi_value MatlabEngineJS::get_buffer_size(napi_env env)
{
  napi_value rval = nullptr;
  if (napi_create_double(env, (double)eng_.getBufferSize(), &rval) != napi_ok)
    napi_throw_error(env, "", "napi_create_double() failed.");
  return rval;
}

napi_value MatlabEngineJS::get_buffer(napi_env env)
{
  const std::string &buf = eng_.getBuffer();
  napi_value rval = nullptr;
  if (buf.empty() || buf[0] == '\0')
  {
    if (napi_get_null(env, &rval) != napi_ok)
      napi_throw_error(env, "", "napi_get_null() failed.");
  }
  else
  {
    if (napi_create_string_utf8(env, buf.c_str(), NAPI_AUTO_LENGTH, &rval) != napi_ok)
      napi_throw_error(env, "", "napi_create_string_utf8() failed.");
  }
  return rval;
}

void MatlabEngineJS::set_visible(napi_env env, napi_value value)
{
  eng_.setVisible(value2bool(env, value));
}

void MatlabEngineJS::set_buffer_enabled(napi_env env, napi_value value)
{
  eng_.setBufferEnabled(value2bool(env, value));
}

void MatlabEngineJS::set_buffer_size(napi_env env, napi_value value)
{
  eng_.setBufferSize(value2uint32(env, value));
}

napi_value MatlabEngineJS::eval(napi_env env, napi_value jsexpr)
{
  // evaluate the expression
  const std::string &strout = eng_.eval(napi_get_value_string_utf8(env, jsexpr));

  napi_value rval;
  if (eng_.getBufferEnabled()) // output returned
  {
    if (napi_create_string_utf8(env, eng_.getBuffer().c_str(), NAPI_AUTO_LENGTH, &rval) != napi_ok)
      throw std::runtime_error("Failed to create string output.");
  }
  else // output not returned
  {
    if (napi_get_undefined(env, &rval) != napi_ok)
      throw std::runtime_error("Failed to create undefined output.");
  }
  return rval;
}

napi_value MatlabEngineJS::get_variable(napi_env env, napi_value jsname)
{

  // get the variable from MATLAB
  managedMxArray val(eng_.getVariable(napi_get_value_string_utf8(env, jsname).c_str()), mxDestroyArray);
  if (!val)
    throw std::runtime_error("Failed to retrieve the requested Matlab variable.");

  // convert mxArray to napi_value
  return mxArrayToNapiValue(env, val.get());

  // // create mxarray object and return it...
  // napi_value cons;
  // status = napi_get_reference_value(env, MatlabMxArray::constructor, &cons);
  // assert(status == napi_ok);

  // // call this function again but invoked as constructor
  // napi_value instance;
  // status = napi_new_instance(env, cons, 0, nullptr, &instance);
  // assert(status == napi_ok);

  // MatlabMxArray *array;
  // status = napi_unwrap(env, instance, reinterpret_cast<void **>(&array));
  // assert(status == napi_ok);
  // array->setMxArray(val); // responsible to delete mxArray

  // return instance;
}

void MatlabEngineJS::put_variable(napi_env env, napi_value jsname, napi_value jsvalue)
{
  // variable name
  std::string var_name = napi_get_value_string_utf8(env, jsname);
  
  // convert to mxArray
  managedMxArray val(napiValueToMxArray(env, jsvalue), mxDestroyArray);

  // get the variable from MATLAB
  eng_.putVariable(var_name.c_str(), val.get());
}
