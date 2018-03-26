#include "matlab-engine.h"
#include "matlab-mxarray.h"
#include "napi_utils.h"

#include <stdexcept>
#include <string>

#include <fstream>
std::ofstream os("test.txt", std::fstream::out);

napi_ref MatlabEngine::constructor;
std::map<double, MatlabEngine::MATLAB_ENGINES> MatlabEngine::sessions;

MatlabEngine::MatlabEngine(double id)
    : id_(id), env_(nullptr), wrapper_(nullptr)
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
      DECLARE_NAPI_METHOD("evaluate", MatlabEngine::evaluate),
      DECLARE_NAPI_METHOD("getVariable", MatlabEngine::get_variable),
      DECLARE_NAPI_METHOD("putVariable", MatlabEngine::put_variable),
      DECLARE_NAPI_METHOD("getVisible", MatlabEngine::get_visible),
      DECLARE_NAPI_METHOD("setVisible", MatlabEngine::set_visible),
      DECLARE_NAPI_METHOD("setOutputBuffer", MatlabEngine::set_output_buffer)};

  //
  napi_value cons;
  status = napi_define_class(env, "MatlabEngine", NAPI_AUTO_LENGTH, MatlabEngine::create,
                             nullptr, 6, properties, &cons);
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

  // check how the function is invoked
  napi_value target;
  napi_status status = napi_get_new_target(env, info, &target);
  assert(status == napi_ok);
  if (target != nullptr) // Invoked as constructor: `new MatlabEngine(...)`
  {
    // retrieve details about the call
    size_t argc = 1; // one optional argument: <double> id
    napi_value args[1];
    napi_value jsthis;
    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok && argc < 2); // only 0 or 1 arguments allowed

    // get id argument if given
    double id = 0.0; // default value
    napi_valuetype valuetype;
    status = napi_typeof(env, args[0], &valuetype);
    assert(status == napi_ok);
    if (valuetype != napi_undefined) // if argument given and is unde
    {
      status = napi_get_value_double(env, args[0], &id);
      assert(status == napi_ok);
    }

    // instantiate new class object
    MatlabEngine *obj;
    try
    {
      obj = new MatlabEngine(id);
    }
    catch (...)
    {
      napi_throw_error(env, "", "Failed to instantiate MatlabEngine");
    }
    obj->env_ = env; // setremember object's environment

    // Wraps the new native instance in a JavaScript object.
    status = napi_wrap(env, jsthis, obj, MatlabEngine::Destructor, nullptr, &obj->wrapper_);
    assert(status == napi_ok);

    return jsthis;
  }
  else // Invoked as plain function `MatlabEngine(...)`, turn into construct call.
  {
    size_t argc_ = 1;
    napi_value args[1];
    status = napi_get_cb_info(env, info, &argc_, args, nullptr, nullptr);
    assert(status == napi_ok);

    const size_t argc = 1;
    napi_value argv[argc] = {args[0]};

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

// MatlabEngine.evaluate(expr)
napi_value MatlabEngine::evaluate(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve details about the call
  size_t argc = 1; // one argument: <string> expr
  napi_value args[1];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok && argc == 1); // only 0 or 1 arguments allowed

  // create expression string
  std::string expr = napi_get_value_string_utf8(env, args[0]);

  // grab the class instance
  MatlabEngine *obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));
  assert(status == napi_ok);

  // clear output buffer (if enabled)
  if (obj->output.size())
    std::fill(obj->output.begin(), obj->output.end(), 0);

  // run MATLAB
  assert(!engEvalString(obj->ep_, expr.c_str())); // only fails if engine session is invalid

  // retrieve the output
  if (obj->output.size())
  {
    napi_value rval;
    status = napi_create_string_utf8(env, obj->output.c_str(), NAPI_AUTO_LENGTH, &rval);
    assert(status == napi_ok);
    return rval;
  }

  return nullptr;
}

// val = getVariable(name) returns mxArray-wrapper object
napi_value MatlabEngine::get_variable(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve details about the call
  size_t argc = 1; // one argument: <string> expr
  napi_value args[1];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok && argc == 1); // must be given one 1 argument

  // create expression string
  std::string name = napi_get_value_string_utf8(env, args[0]);

  // grab the class instance
  MatlabEngine *obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));
  assert(status == napi_ok);

  // get the variable from MATLAB
  mxArray *val = engGetVariable(obj->ep_, name.c_str());
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

// obj.putVariable(name, val) place the given node.js mxArray object onto MATLAB
// workspace (values will not be synced)
napi_value MatlabEngine::put_variable(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve details about the call
  size_t argc = 2; // one argument: <string> expr
  napi_value args[2];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok && argc == 2); // must be given 2 arguments allowed

  // get variable name
  std::string name = napi_get_value_string_utf8(env, args[0]);

  // get mxArray variable value
  napi_value var_value;
  status = napi_get_reference_value(env, MatlabMxArray::constructor, &var_value);
  bool ok;
  status = napi_instanceof(env, args[1], var_value, &ok);
  assert(status == napi_ok && ok);
  MatlabMxArray *var;
  status = napi_unwrap(env, args[1], reinterpret_cast<void **>(&var));

  // grab the class instance
  MatlabEngine *obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));
  assert(status == napi_ok);

  // get the variable from MATLAB
  const mxArray *data = var->getMxArray();
  assert(!engPutVariable(obj->ep_, name.c_str(), var->getMxArray()));
  
  return nullptr;
}

// tf = obj.getVisible(name) returns true if MATLAB window is visible
napi_value MatlabEngine::get_visible(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve details about the call
  size_t argc = 0; // one argument: <string> expr
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, nullptr, &jsthis, nullptr);
  assert(status == napi_ok && argc == 0);

  // grab the class instance
  MatlabEngine *obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));
  assert(status == napi_ok);

  // get the variable from MATLAB
  bool tf;
  assert(!engGetVisible(obj->ep_, &tf));

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

  // retrieve details about the call
  size_t argc = 1; // one argument: <string> expr
  napi_value args[1];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok && argc == 1); // must be given 2 arguments allowed

  // get mxArray variable value
  bool onoff;
  status = napi_get_value_bool(env, args[0], &onoff);

  // grab the class instance
  MatlabEngine *obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));
  assert(status == napi_ok);

  // get the variable from MATLAB
  engSetVisible(obj->ep_, onoff);

  return nullptr;
}

// obj.setOutputBufferSize(sz) place the given node.js mxArray object onto MATLAB
// workspace (values will not be synced)
napi_value MatlabEngine::set_output_buffer(napi_env env, napi_callback_info info)
{
  napi_status status;

  // retrieve details about the call
  size_t argc = 1; // one argument: <string> expr
  napi_value args[1];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok && argc == 1); // must be given 2 arguments allowed

  // get number
  int32_t size;
  status = napi_get_value_int32(env, args[0], &size);
  assert(status == napi_ok && size >= 0);

  // grab the class instance
  MatlabEngine *obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&obj));
  assert(status == napi_ok);

  // update string size
  obj->output.resize(size+1);
  engOutputBuffer(obj->ep_, obj->output.data(), size);

  return nullptr;
}
