// defines an native addon node.js object to interface with MATLAB engine
//    .evaluate(expr)
//    .setVariable(value)
//    .putVariable
//    .getVariable
//    .setOutputBuffer

#pragma once

#include <engine.h>
#include <node_api.h>

#include <map>
#include <string>

class MatlabEngine
{
public:
  static napi_value Init(napi_env env, napi_value exports);

  static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);

  static napi_ref constructor;

private:
  /**
 * \brief Constructor
 * 
 * Create a new MATLAB Engine session if none exists and returns the pointer
 * to the engine session as the node.js external data object. Multiple 
 * sessions of MATLAB could be opened by using different \ref id.
 * 
 * \param[in] session id number to support multiple MATLAB sessions
 */
  explicit MatlabEngine(double id_ = 0);

   /**
 * \brief Destrctor
 * 
 * Release the assigned MATLAB session
 */
 ~MatlabEngine();

/**
 * \brief Create new MatlabEngine object
 * 
 * \param[in] env  The environment that the API is invoked under.
 * \param[in] info The callback info passed into the callback function.
 * \returns napi_value representing the JavaScript object returned, which in 
 *          this case is the constructed object.
 */
  static napi_value create(napi_env env, napi_callback_info info);

  /**
 * \brief Evluates MATLAB expression
 */
  static napi_value evaluate(napi_env env, napi_callback_info info);

  /**
 * \brief Copy variable from MATLAB engine workspace
 * 
 * An object factory function
 */
  static napi_value get_variable(napi_env env, napi_callback_info info);

  /**
 * \brief Put variable into MATLAB engine workspace
 */
  static napi_value put_variable(napi_env env, napi_callback_info info);

  /**
 * \brief Determine visibility of MATLAB engine session
 */
  static napi_value get_visible(napi_env env, napi_callback_info info);

  /**
 * \brief Show or hide MATLAB engine session
 */
  static napi_value set_visible(napi_env env, napi_callback_info info);

  /**
 * \brief Specify buffer for MATLAB output
 */
  static napi_value set_output_buffer(napi_env env, napi_callback_info info);

  /**
 * \brief Retrieve the output buffer from MATLAB
 */
  static napi_value get_output_buffer(napi_env env, napi_callback_info info);

  struct MATLAB_ENGINES
  {
    Engine *ep;
    int count;
  };

  static std::map<double, MATLAB_ENGINES> sessions;
  
  double id_; // MATLAB engine session id
  Engine *ep_;
  std::string output;

  napi_env env_;
  napi_ref wrapper_;
};
