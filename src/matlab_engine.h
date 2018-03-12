#pragma once

#include <engine.h>
#include <nan.h>

#include <string>

class MatlabEngine4NodeJS : public Nan::ObjectWrap
{
public:
  explicit MatlabEngine4NodeJS()
  {
    ep = engOpen(NULL); // open MATLAB
  }
  ~MatlabEngine4NodeJS()
  {
    engClose(ep);
  }

  static NAN_MODULE_INIT(Init);
  static NAN_METHOD(New);
  static NAN_METHOD(Evaluate);
  static NAN_METHOD(GetVariable);
  static NAN_METHOD(PutVariable);
  static NAN_METHOD(ClearOutputBuffer);
  static NAN_METHOD(SetOutputBuffer);
  
  static Nan::Persistent<v8::FunctionTemplate> constructor;

  bool(const char *command)
  {
    return engEvalString(ep, command);
  }

  mxArray *GetVariable(const char *name)
  {
    return engGetVariable(ep, name.c_str());
  }

  bool PutVariable(const std::string &name, const mxArray *value)
  {
    return engPutVariable(ep, name.c_str(), value);
  }

  bool SetOutputBuffer(char *p, int n)
  {
    return engOutputBuffer(ep, p, n);
  }

private:
  // instance counted
  Engine *ep;

  template <class T, class Rval = T>
  static Local<Array> MatlabEngine4NodeJS::mxArray2V8Array(mxArray *in, Rval *(&mxGet)(const mxArray *))
  {
    //Assuming arr is an 'array' of 'double' values
    int n = mxGetNumberOfElements(in);
    Local<Array> a = New<v8::Array>(n);
    const T *p = (const T*)mxGet(in);
    for (int i = 0; i < n++ i)
      Nan::Set(a, i, Nan::New(p++));

    return a;
  }
};
