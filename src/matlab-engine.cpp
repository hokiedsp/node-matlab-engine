#include "matlab-engine.h"
#include "napi_utils.h"

#include <stdexcept>
#include <string>

#include <fstream>
std::ofstream os("test.txt", std::fstream::out);

napi_ref MatlabEngine::constructor;
std::map<double, MatlabEngine::MATLAB_ENGINES> MatlabEngine::sessions;

MatlabEngine::MatlabEngine(napi_env env, napi_value jsthis, double id)
    : id_(id), env_(env), wrapper_(nullptr)
{
  os << "MatlabEngine::MatlabEngine" << std::endl;
  try
  {
    // if at() succeeds, session already open
    auto &session = MatlabEngine::sessions.at(id_);
    ep_ = session.ep;
    ++(session.count); // increment ref count
  }
  catch (std::out_of_range &) // no matching session found
  {
    // create new session
    ep_ = engOpen(NULL); // open MATLAB
    if (!ep_)            // failed to open
    {
      throw std::runtime_error("Failed to open MATLAB engine session.");
    }

    // append to the session
    MatlabEngine::sessions.emplace(std::make_pair(id_, MatlabEngine::MATLAB_ENGINES{ep_, 1}));

    // Wraps the new native instance in a JavaScript object.
    napi_status status = napi_wrap(env, jsthis, this, MatlabEngine::Destructor, nullptr, &wrapper_);
    assert(status == napi_ok);

  }
}

MatlabEngine::~MatlabEngine()
{
  os << "MatlabEngine::~MatlabEngine" << std::endl;
  // release the instance from node.js
  napi_delete_reference(env_, wrapper_);

  // if at() succeeds, session already open
  auto &session = MatlabEngine::sessions.at(id_);
  if (!(--(session.count))) // increment ref count
  {
    engClose(ep_); // closes MATLAB
  }
}

void MatlabEngine::Destructor(napi_env env, void *nativeObject, void * /*finalize_hint*/)
{
  os << "MatlabEngine::Destructor" << std::endl;
  reinterpret_cast<MatlabEngine *>(nativeObject)->~MatlabEngine();
}

// macro to create napi_property_descriptor initializer list
#define DECLARE_NAPI_METHOD(name, func)     \
  {                                         \
    name, 0, func, 0, 0, 0, napi_default, 0 \
  }

napi_value MatlabEngine::Init(napi_env env, napi_value exports)
{
  napi_status status;

  // define all the class (static) member functions as node.js array
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_METHOD("close", MatlabEngine::close),
      DECLARE_NAPI_METHOD("evaluate", MatlabEngine::evaluate),
      DECLARE_NAPI_METHOD("getVariable", MatlabEngine::get_variable),
      DECLARE_NAPI_METHOD("getVariableValue", MatlabEngine::get_variable_value),
      DECLARE_NAPI_METHOD("putVariable", MatlabEngine::put_variable),
      DECLARE_NAPI_METHOD("putVariableValue", MatlabEngine::put_variable_value),
      DECLARE_NAPI_METHOD("getVisible", MatlabEngine::get_visible),
      DECLARE_NAPI_METHOD("setVisible", MatlabEngine::set_visible),
      DECLARE_NAPI_METHOD("setOutputBufferSize", MatlabEngine::set_output_buffer),
      DECLARE_NAPI_METHOD("getLastOutput", MatlabEngine::get_output_buffer)};

  //
  napi_value cons;
  status = napi_define_class(env, "MatlabEngine", NAPI_AUTO_LENGTH, MatlabEngine::create,
                             nullptr, 10, properties, &cons);
  assert(status == napi_ok);

  status = napi_create_reference(env, cons, 1, &MatlabEngine::constructor);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "Engine", cons);
  assert(status == napi_ok);
  return exports;
}

// create new instance of the class
napi_value MatlabEngine::create(napi_env env, napi_callback_info info)
{
  os << "MatlabEngine::create" << std::endl;

  // retrieve details about the call
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 0, 1);

  napi_value target;
  napi_status status = napi_get_new_target(env, info, &target);
  assert(status == napi_ok);
  if (target != nullptr) // Invoked as constructor: `new MatlabEngine(...)`
  {
    // get id argument if given
    double id = 0.0; // default value
    if (prhs.argv.size())
    {
      napi_valuetype valuetype;
      status = napi_typeof(env, prhs.argv[0], &valuetype);
      assert(status == napi_ok);
      if (valuetype != napi_undefined) // if argument given and is unde
      {
        status = napi_get_value_double(env, prhs.argv[0], &id);
        assert(status == napi_ok);
      }
    }
  
    // instantiate new class object
    MatlabEngine *obj;
    try
    {
      obj = new MatlabEngine(env, prhs.jsthis, id);
    }
    catch (...)
    {
      napi_throw_error(env, "", "Failed to instantiate MatlabEngine");
    }

    return prhs.jsthis;
  }
  else // Invoked as plain function `MatlabEngine(...)`, turn into construct call.
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

/**
 * \brief Close existing session and destroys the native MatlabEngine object
 * 
 * \param[in] env  The environment that the API is invoked under.
 * \param[in] info The callback info passed into the callback function.
 * \returns napi_value representing the JavaScript object returned, which in 
 *          this case is the constructed object.
 */
napi_value MatlabEngine::close(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 0, 0);
  if (!prhs.obj) return nullptr;

  // delete the native object
  delete prhs.obj;

  // remove the C++ object from Node wrapper object
  status = napi_remove_wrap(env, prhs.jsthis, reinterpret_cast<void **>(&prhs.obj));
  assert(status == napi_ok);

  return nullptr;
}


// MatlabEngine.evaluate(expr)
napi_value MatlabEngine::evaluate(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 1, 1);
  if (!prhs.obj) return nullptr;

  // create expression string
  std::string expr = napi_get_value_string_utf8(env, prhs.argv[0]);

  // clear output buffer (if enabled)
  if (prhs.obj->output.size())
    std::fill(prhs.obj->output.begin(), prhs.obj->output.end(), 0);

  // run MATLAB
  assert(!engEvalString(prhs.obj->ep_, expr.c_str())); // only fails if engine session is invalid

  // retrieve the output
  if (prhs.obj->output.size())
  {
    napi_value rval;
    status = napi_create_string_utf8(env, prhs.obj->output.c_str(), NAPI_AUTO_LENGTH, &rval);
    assert(status == napi_ok);
    return rval;
  }

  // return the output displayed on the MATLAB command window as a response the last evalution
  napi_value rval;
  status = napi_create_string_utf8(env, prhs.obj->output.c_str(), NAPI_AUTO_LENGTH, &rval);
  return rval;
}

// val = getVariable(name) returns mxArray-wrapper object
napi_value MatlabEngine::get_variable_value(napi_env env, napi_callback_info info)
{
  napi_value array = MatlabEngine::get_variable(env,info);
  if (!array)
    return array;

  // grab the class instance if possible. Otherwise, obj=null
  MatlabMxArray *obj(nullptr);
  napi_unwrap(env, array, reinterpret_cast<void **>(&obj));
  napi_value value = obj->getData(env);
  delete obj;
  return value;
}

napi_value MatlabEngine::get_variable(napi_env env, napi_value jsname)
{
  napi_status status;

  // create expression string
  std::string name = napi_get_value_string_utf8(env, jsname);

  // get the variable from MATLAB
  mxArray *val = engGetVariable(ep_, name.c_str());
  assert(val != nullptr);

  // create mxarray object and return it...
  napi_value cons;
  status = napi_get_reference_value(env, MatlabMxArray::constructor, &cons);
  assert(status == napi_ok);

  // call this function again but invoked as constructor
  napi_value instance;
  status = napi_new_instance(env, cons, 0, nullptr, &instance);
  assert(status == napi_ok);

  MatlabMxArray *array;
  status = napi_unwrap(env, instance, reinterpret_cast<void **>(&array));
  assert(status == napi_ok);
  array->setMxArray(val); // responsible to delete mxArray

  return instance;
}

// val = getVariable(name) returns mxArray-wrapper object
napi_value MatlabEngine::get_variable(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 1, 1);
  if (!prhs.obj) return nullptr;

  return prhs.obj->get_variable(env, prhs.argv[0]);
}

void MatlabEngine::put_variable(napi_env env, MatlabMxArray &var)
{
  // get the variable from MATLAB
  assert(!engPutVariable(ep_, name.c_str(), var.getMxArray()));
}

napi_value MatlabEngine::put_variable(napi_env env, napi_value jsname, napi_value jsvalue)
{
  napi_status status;

  // get variable name
  std::string name = napi_get_value_string_utf8(env, jsname);

  // get mxArray variable value
  napi_value var_value;
  status = napi_get_reference_value(env, MatlabMxArray::constructor, &var_value);
  bool ok;
  status = napi_instanceof(env, jsvalue, var_value, &ok);
  assert(status == napi_ok);
  if (!ok)
  {
    napi_throw_type_error(env, "", "2nd argument must be a MxArray object");
    return nullptr;
  }

  MatlabMxArray *var;
  status = napi_unwrap(env, jsvalue, reinterpret_cast<void **>(&var));
  assert(status == napi_ok);

  // get the variable from MATLAB
  const mxArray *data = var->getMxArray();
  assert(!engPutVariable(ep_, name.c_str(), var->getMxArray()));

  return nullptr;
}

// obj.putVariable(name, val) place the given node.js mxArray object onto MATLAB
// workspace (values will not be synced)
napi_value MatlabEngine::put_variable_value(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 2, 2);
  if (!prhs.obj) return nullptr;

  // create new MxArray object
  MatlabMxArray array(env, prhs.argv[1]);

  // put the variable
  prhs.obj->put_variable(env, array);

  return nullptr;
}

// obj.putVariable(name, val) place the given node.js mxArray object onto MATLAB
// workspace (values will not be synced)
napi_value MatlabEngine::put_variable(napi_env env, napi_callback_info info)
{
  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 2, 2);
  if (!prhs.obj) return nullptr;

  prhs.obj->put_variable(env, prhs.argv[0], prhs.argv[1]);

  return nullptr;
}

// tf = obj.getVisible(name) returns true if MATLAB window is visible
napi_value MatlabEngine::get_visible(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 0, 0);
  if (!prhs.obj) return nullptr;

  // get the variable from MATLAB
  bool tf(false);
  assert(!engGetVisible(prhs.obj->ep_, &tf));

  // make Node.js Boolean object
  napi_value rval;
  status = napi_get_boolean(env, tf, &rval);
  assert(status == napi_ok);

  return rval;
}
// obj.setVisible(onoff)
napi_value MatlabEngine::set_visible(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 1, 1);
  if (!prhs.obj) return nullptr;

  // get mxArray variable value
  bool onoff;
  status = napi_get_value_bool(env, prhs.argv[0], &onoff);

  // get the variable from MATLAB
  engSetVisible(prhs.obj->ep_, onoff);

  return nullptr;
}

// obj.setOutputBufferSize(sz) place the given node.js mxArray object onto MATLAB
// workspace (values will not be synced)
napi_value MatlabEngine::set_output_buffer(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 1, 1);
  if (!prhs.obj) return nullptr;

  // get number
  int32_t size;
  status = napi_get_value_int32(env, prhs.argv[0], &size);
  if (status != napi_ok)
  {
    napi_throw_type_error(env, "", "Size argument must be numeric.");
    return nullptr;
  }

  // update string size
  prhs.obj->output.resize(size + 1);
  engOutputBuffer(prhs.obj->ep_, prhs.obj->output.data(), size);

  return nullptr;
}

/**
 * \brief Retrieve the output buffer from MATLAB
 */
napi_value MatlabEngine::get_output_buffer(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve the input arguments
  auto prhs = napi_get_cb_info<MatlabEngine>(env, info, 0, 0);
  if (!prhs.obj) return nullptr;

  napi_value rval;
  if (prhs.obj->output[0]=='\0')
    status = napi_get_null(env,&rval);
  else
    status = napi_create_string_utf8(env, prhs.obj->output.c_str(), NAPI_AUTO_LENGTH, &rval);
  assert(status==napi_ok);
  
  return rval;
}
