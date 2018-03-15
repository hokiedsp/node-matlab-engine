// <nan.h>
// Nan::NAN_METHOD_RETURN_TYPE name(Nan::NAN_METHOD_ARGS_TYPE info)
// typedef const FunctionCallbackInfo<v8::Value>& NAN_METHOD_ARGS_TYPE;
// <v8.h>
// FunctionCallbackInfo: https://v8.paulfryzel.com/docs/master/classv8_1_1_function_callback_info.html
// V8_INLINE int 	Length () const 
// V8_INLINE Local< Value > 	operator[] (int i) const // arguments
// V8_INLINE Local< Function > 	Callee () const
// V8_INLINE Local< Object > 	This () const
// V8_INLINE Local< Object > 	Holder () const
// V8_INLINE bool 	IsConstructCall () const
// V8_INLINE Local< Value > 	Data () const
// V8_INLINE Isolate * 	GetIsolate () const
// V8_INLINE ReturnValue< T > 	GetReturnValue () const 
//
// v8::Local: https://v8.paulfryzel.com/docs/master/classv8_1_1_local.html
// v8::Value: https://v8.paulfryzel.com/docs/master/classv8_1_1_value.html
// v8::ReturnValue: https://v8.paulfryzel.com/docs/master/classv8_1_1_return_value.html

// typedef void NAN_METHOD_RETURN_TYPE;


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

  // bool Evaluate(const char *command)
  // {
  //   return engEvalString(ep, command);
  // }

  // mxArray *GetVariable(const char *name)
  // {
  //   return engGetVariable(ep, name);
  // }

  // bool PutVariable(const std::string &name, const mxArray *value)
  // {
  //   return engPutVariable(ep, name.c_str(), value);
  // }

  // bool SetOutputBuffer(char *p, int n)
  // {
  //   return engOutputBuffer(ep, p, n);
  // }

private:
  // instance counted
  Engine *ep;

  template <class T, class MXGET>
  static v8::Local<v8::Array> mxArray2V8Array(mxArray *in, MXGET* mxGet)
  {
    //Assuming arr is an 'array' of 'double' values
    int n = mxGetNumberOfElements(in);
    v8::Local<v8::Array> a = New<v8::Array>(n);
    const T *p = (const T*)mxGet(in);
    for (int i = 0; i < n++ i)
      Nan::Set(a, i, Nan::New(p++));

    return a;
  }
};
