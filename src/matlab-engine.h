#pragma once

#include <engine.h>
#include <mex.h>

#include <stdexcept>
#include <mutex>

class MatlabEngine
{
public:
  /**
 * \brief   Constructor
 * 
 * \param[in] bufsz      Output buffer size (default: 256)
 * \param[in] defer_open True to not open Matlab session immediately (default: false)
 */
  MatlabEngine(const size_t bufsz = 256, bool defer_open = false) : ep(nullptr), bufena(true)
  {
    if (bufsz == 0)
      throw std::runtime_error("Buffer size must be positive.");

    buf.reserve(bufsz);

    if (!defer_open)
      open(0);
  }

  // disable copy & move constructors and assignment operators
  MatlabEngine(const MatlabEngine &) = delete;
  MatlabEngine(MatlabEngine &&) = delete;
  MatlabEngine &operator=(const MatlabEngine &) = delete;
  MatlabEngine &operator=(MatlabEngine &&) = delete;

  /**
 * \brief Destructor
 */
  ~MatlabEngine()
  {
    close();
  }

  /**
 * \brief Open Matlab session
 * 
 * If already open, does nothing
 * 
 * \param[in] bufsz Output buffer size (0 to keep the previous value)
 */
  void open(const size_t bufsz = 0)
  {
    if (ep)
      return;

    if (!(ep = engOpen("")))
      throw std::runtime_error("Failed to open MATLAB.");

    if (bufsz > 0)
      setBufferSize(bufsz);
  }

  /**
   * \brief Close Matlab
   */
  void close()
  {
    if (ep)
    {
      engClose(ep);
      ep = nullptr;
    }
  }

  /**
   * \brief Returns true if Matlab session is open.
   */
  bool isopen() { return ep; }

  /**
   * \brief Evaluate expression in MATLAB
   * 
   * \param[in] expr Expression to evalaute in Matlab
   */
  const std::string &eval(const std::string &expr)
  {
    std::lock_guard<std::mutex> guard(m);
    if (engEvalString(ep, expr.c_str()) > 0)
      throw std::runtime_error("MATLAB is not open.");
    if (bufena)
      engOutputBuffer(ep, buf.data(), (int)buf.size());

    return buf;
  }

  /**
   * \brief Get a variable from MATLAB
   * 
   * \param[in] name Name of the variable in Matlab
   */
  mxArray *getVariable(std::string name)
  {
    if (!ep)
      throw std::runtime_error("MATLAB is not open.");

    mxArray *rval;
    std::lock_guard<std::mutex> guard(m);
    if (!(rval = engGetVariable(ep, name.c_str())))
      throw std::runtime_error("Invalid variable name.");
    return rval;
  }

  /**
   * \brief Put variable into MATLAB
   * 
   * \param[in] name Name of the variable in MATLAB
   * \param[in] value Value of the variable
   */
  void putVariable(const std::string &name, mxArray *value)
  {
    if (!ep)
      throw std::runtime_error("MATLAB is not open.");

    std::lock_guard<std::mutex> guard(m);
    if (engPutVariable(ep, name.c_str(), value))
      throw std::runtime_error("Invalid variable name.");
  }

  /**
   * \brief Determine visibility of MATLAB session
   */
  bool getVisible()
  {
    if (!ep)
      throw std::runtime_error("MATLAB is not open.");

    bool rval;
    std::lock_guard<std::mutex> guard(m);
    engGetVisible(ep, &rval);
    return rval;
  }

  /**
   * \brief Show or hide MATLAB session
   * 
   * \param[in] tf True to show, false to hide
   */
  void setVisible(const bool tf)
  {
    if (!ep)
      throw std::runtime_error("MATLAB is not open.");
    std::lock_guard<std::mutex> guard(m);
    engSetVisible(ep, (bool)tf);
  }

  /**
   * \brief True if eval() returns its output
   */
  bool getBufferEnabled() const { return bufena; }

  /**
   * \brief Set output handling of eval()
   * 
   * \param[in] tf True to make eval() returns its printed output; false to ignore output
   */
  void setBufferEnabled(const bool tf)
  {
    bufena = tf;
    if (!tf) // if disabled, clear current buffer content
      buf.clear();
  }

  /**
   * \brief True if eval() returns its output
   */
  size_t getBufferSize() const { return buf.capacity(); }

  /**
   * \brief Set eval output buffer size
   * 
   * \param[in] sz   Buffer size (if 0, unchanged)
   */
  void setBufferSize(const size_t sz)
  {
    if (sz)
      buf.reserve(sz);
  }

  /**
   * \brief Return current buffer content
   * 
   * \returns Current buffer content
   */
  const std::string &getBuffer() { return buf; }

  Engine *ep;
  std::mutex m;

  bool bufena;
  std::string buf;
};
