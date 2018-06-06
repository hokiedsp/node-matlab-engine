// defines an native addon node.js object to interface with MATLAB engine
//    .evaluate(expr)
//    .setVariable(value)
//    .putVariable
//    .getVariable
//    .setOutputBuffer

#pragma once

#include "matlab-engine.h"
// #include "matlab-mxarray.h"

#include <node_api.h>

#include <map>
#include <string>

/**
 * MatlabEngineJS   Interface between JS and Matlab
 * 
 * Init - export 
 * Create     - create new MatlabEngine object
 * Destructor - destroy MatlabEngine object
 * ****** PROTYPE FUNCTIONS ******
 * Open   - open/connect to a new Matlab session
 * Close  - close/disconnect the existing Matlab session
 * EvalSync - Synchronous Matlab expression evaluation
 * Eval   - Asynchronous Matlab expression evaluation
 * PutVariable - Place given variable onto Matlab workspace
 * GetVariable - Get specified variable from Matlab workspace
 * FevalSync - Synchronous m-function evaluation
 * Feval  - Asynchronous m-function evaluation
 * ******* PROTOTYPE VARIABLES ******
 * IsOpen - returns True if Matalb session is open
 * Visible     - true if visible (setVisible, getVisible)
 * OutputBufferEnabled - Enable output buffering (setOutputBufferEnabled, getOutputBufferEnabled)
 * OutputBufferSize  - Output buffer size (setOutputBufferSize, getOutputBufferSize)
 * OutputBuffer - Output buffer, read-only
 */
class MatlabEngineJS
{
public:
  static napi_value Init(napi_env env, napi_value exports);

  static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);

  static napi_ref constructor;

private:
  /**
 * \brief Create new MatlabEngine object
 * 
 * \param[in] env  The environment that the API is invoked under.
 * \param[in] info The callback info passed into the callback function.
 * \returns napi_value representing the JavaScript object returned, which in 
 *          this case is the constructed object.
 */
  static napi_value Create(napi_env env, napi_callback_info info);

  /**
 * \brief Open Matlab session
 * 
 * session.Open(id)
 */
  static napi_value Open(napi_env env, napi_callback_info info);

  /**
 * \brief Close existing session and destroys the native MatlabEngine object
 * 
 * session.Close()
 * 
 */
  static napi_value Close(napi_env env, napi_callback_info info);

  /**
 * \brief True if Matlab session is open
 * 
 */
  static napi_value GetIsOpen(napi_env env, napi_callback_info info);

  /**
 * \brief Asynchronously run MATLAB m-function
 * 
 * plhs_promise = session.Feval(nlhs, prhs)
 * 
 */
  // static napi_value Feval(napi_env env, napi_callback_info info);

  /**
 * \brief Asynchronously evluates MATLAB expression
 * 
 * session.Eval(expr)
 * output_promise = session.Eval(expr) - to retrieve output buffer
 * 
 */
  // static napi_value Eval(napi_env env, napi_callback_info info);

  /**
 * \brief Synchronously run MATLAB m-function
 * 
 * plhs = session.FevalSync(nlhs, prhs)
 * 
 */
  // static napi_value FevalSync(napi_env env, napi_callback_info info);

  /**
 * \brief Synchronously evluates MATLAB expression
 * 
 * session.EvalSync(expr)
 * output = session.EvalSync(expr) - to retrieve output buffer
 * 
 */
  static napi_value EvalSync(napi_env env, napi_callback_info info);

  /**
 * \brief Copy variable from MATLAB engine workspace
 * 
 * value = session.GetVariable(name)
 */
  static napi_value GetVariable(napi_env env, napi_callback_info info);

  /**
 * \brief Put variable into MATLAB engine workspace
 * session.PutVariable(name, value)
 */
  static napi_value PutVariable(napi_env env, napi_callback_info info);

  /**
 * \brief Getter for session.Visible property
 */
  static napi_value GetVisible(napi_env env, napi_callback_info info);

  /**
 * \brief Setter for session.Visible property
 */
  static napi_value SetVisible(napi_env env, napi_callback_info info);

  /**
 * \brief Getter for session.EnableBuffer property
 */
  static napi_value GetBufferEnabled(napi_env env, napi_callback_info info);

  /**
 * \brief Setter for session.EnableBuffer property
 */
  static napi_value SetBufferEnabled(napi_env env, napi_callback_info info);

  /**
 * \brief Specify buffer size to store for MATLAB output
 * 
 * session.SetOutputBuffer
 */
  static napi_value SetBufferSize(napi_env env, napi_callback_info info);

  /**
 * \brief Retrieve the output buffer size to store MATLAB output
 */
  static napi_value GetBufferSize(napi_env env, napi_callback_info info);

  /**
 * \brief Retrieve the output during the last MATLAB evaluation
 */
  static napi_value GetBuffer(napi_env env, napi_callback_info info);

  //////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////

  /**
 * \brief Constructor
 * 
 * Create a new MATLAB Engine session if none exists and returns the pointer
 * to the engine session as the node.js external data object. Multiple 
 * sessions of MATLAB could be opened by using different \ref id.
 */
  explicit MatlabEngineJS(napi_env env, napi_value jsthis, napi_value opt = nullptr);

  /**
   * \brief open MATLAB session
   */
  void open();
  void close();

  napi_value is_open(napi_env env);
  napi_value get_visible(napi_env env);
  void set_visible(napi_env env, napi_value value);
  napi_value get_buffer_enabled(napi_env env);
  void set_buffer_enabled(napi_env env, napi_value value);
  napi_value get_buffer_size(napi_env env);
  void set_buffer_size(napi_env env, napi_value value);
  napi_value get_buffer(napi_env env);

  napi_value eval(napi_env env, napi_value jsexpr);

  napi_value get_variable(napi_env env, napi_value jsname);

  void put_variable(napi_env env, napi_value jsname, napi_value jsvalue);

  /**
 * \brief Put variable into MATLAB engine workspace
 */
  // void put_variable(napi_env env, const std::string &name, MatlabMxArray &array);

  MatlabEngine eng_;

  napi_ref wrapper_;
};
