#include "matlab_engine.h"

Nan::Persistent<v8::FunctionTemplate> MatlabEngine4NodeJS::constructor;

NAN_MODULE_INIT(MatlabEngine4NodeJS::Init)
{
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(MatlabEngine4NodeJS::New);
  constructor.Reset(ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(Nan::New("MatlabEngine4NodeJS").ToLocalChecked());

  Nan::SetPrototypeMethod(ctor, "evaluate", Evaluate);
  Nan::SetPrototypeMethod(ctor, "get_variable", GetVariable);
  Nan::SetPrototypeMethod(ctor, "put_variable", PutVariable);
  Nan::SetPrototypeMethod(ctor, "set_output_buffer", SetOutputBuffer);
  Nan::SetPrototypeMethod(ctor, "clear_output_buffer", ClearOutputBuffer);
  
  target->Set(Nan::New("MatlabEngine4NodeJS").ToLocalChecked(), ctor->GetFunction());
}

NAN_METHOD(MatlabEngine4NodeJS::New)
{
  // throw an error if constructor is called without new keyword
  if (!info.IsConstructCall())
  {
    return Nan::ThrowError(Nan::New("MatlabEngine4NodeJS::New - called without new keyword").ToLocalChecked());
  }

  // expect no arguments
  if (info.Length() != 0)
  {
    return Nan::ThrowError(Nan::New("MatlabEngine4NodeJS::New - expected no argument").ToLocalChecked());
  }

  // create a new instance and wrap our javascript instance
  MatlabEngine4NodeJS *eng = new MatlabEngine4NodeJS();
  eng->Wrap(info.Holder());

  // return the wrapped javascript instance
  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(MatlabEngine4NodeJS::Evaluate)
{
  // expect 1 arguments
  if (info.Length() != 1)
    return Nan::ThrowError(Nan::New("Expected 1 argument").ToLocalChecked());

  // unwrap this MatlabEngine4NodeJS
  MatlabEngine4NodeJS *self = Nan::ObjectWrap::Unwrap<MatlabEngine4NodeJS>(info.This());

  // get the MATLAB string expression to evaluate
  Nan::Utf8String utf8_value(info[0]);
  int len = utf8_value.length();
  if (len <= 0)
    return Nan::ThrowTypeError("Arg must be a non-empty string");
  std::string buffer(*utf8_value, len);

  // run the MATLAB && return its return value to JS
  info.GetReturnValue().Set(Nan::New(engEvalString(self->ep, buffer.c_str())));
}

NAN_METHOD(MatlabEngine4NodeJS::GetVariable)
{
  // expect 1 arguments
  if (info.Length() != 1)
    return Nan::ThrowError(Nan::New("Expected only 1 argument").ToLocalChecked());

  // unwrap this MatlabEngine4NodeJS
  MatlabEngine4NodeJS *self = Nan::ObjectWrap::Unwrap<MatlabEngine4NodeJS>(info.This());

  // grab the variable name
  Nan::Utf8String utf8_value(info[0]);
  int len = utf8_value.length();
  if (len <= 0)
    return Nan::ThrowTypeError("arg must be a non-empty string");
  std::string name(*utf8_value, len);

  mxArray *val = engGetVariable(self->ep, name.c_str());
  if (!val)
    return Nan::ThrowTypeError("arg must be a name of existing MATLAB variable name");

  if (mxIsComplex(val))
    return Nan::ThrowTypeError("Only real-valued MATLAB variable can be retrieved.");

  if (mxIsEmpty(val))
  {
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  if (mxGetNumberOfDimensions(val) > 2 || !(mxGetM(val) == 1 || mxGetN(val) == 1))
    return Nan::ThrowTypeError("Only vector valued MATLAB variables can be retrieved.");

  bool scalar = mxIsScalar(val);
  if (mxIsNumeric(val) && scalar)
  {
    info.GetReturnValue().Set(Nan::New(mxGetScalar(val)));
  }
  else if (mxIsChar(val))
  {
    info.GetReturnValue().Set(Nan::New((const uint16_t *)mxGetChars(val))); // UTF-16
  }
  else if (mxIsLogical(val))
  {
    if (scalar)
      info.GetReturnValue().Set(Nan::New(*mxGetLogicals(val)));
    else
      info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<bool>(val, &mxGetLogicals));
  }
  else if (mxIsDouble(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<double>(val, &mxGetPr));
  }
  else if (mxIsSingle(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<float, void>(val, &mxGetData));
  }
  else if (mxIsInt64(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<int64_t, void>(val, &mxGetData));
  }
  else if (mxIsUint64(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<uint64_t, void>(val, &mxGetData));
  }
  else if (mxIsInt32(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<int32_t, void>(val, &mxGetData));
  }
  else if (mxIsUint32(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<uint32_t, void>(val, &mxGetData));
  }
  else if (mxIsInt16(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<int16_t, void>(val, &mxGetData));
  }
  else if (mxIsUint16(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<uint16_t, void>(val, &mxGetData));
  }
  else if (mxIsInt8(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<int8_t, void>(val, &mxGetData));
  }
  else if (mxIsUint8(val)) // array
  {
    info.GetReturnValue().Set(MatlabEngine4NodeJS::mxArray2V8Array<uint8_t, void>(val, &mxGetData));
  }
  else
    return Nan::ThrowTypeError("Unsupported data type.");
}

NAN_METHOD(MatlabEngine4NodeJS::PutVariable)
{
  // expect 2 arguments
  if (info.Length() != 2)
    return Nan::ThrowError(Nan::New("Expected only 2 argument").ToLocalChecked());

  // unwrap this MatlabEngine4NodeJS
  MatlabEngine4NodeJS *self = Nan::ObjectWrap::Unwrap<MatlabEngine4NodeJS>(info.This());

  // grab the variable name
  Nan::Utf8String utf8_value(info[0]);
  int len = utf8_value.length();
  if (len <= 0)
    return Nan::ThrowTypeError("arg must be a non-empty string");
  std::string name(*utf8_value, len);

  mxArray *val = NULL;
  std::unique_ptr<mxArray, decltype(mxDestroyArray) *> onCleanup(val, mxDestroyArray);

  Nan::MaybeLocal<v8::Value> maybe_value(info[1]);
  if (maybe_value.IsEmpty())
  {
    val = mxCreateDoubleMatrix(0, 0, mxREAL);
  }
  else
  {
    v8::Local<v8::Value> value = maybe_value.ToLocalChecked();
    if (value->IsBooleanObject())
    {
      val = mxCreateLogicalScalar(value->BooleanValue());
    }
    else if (value->IsNumberObject())
    {
      val = mxCreateDoubleScalar(value->NumberValue());
    }
    else if (value->IsStringObject())
    {
      Nan::Utf8String utf8_value(info[1]);
      std::string str_value(*utf8_value, utf8_value.length());
      val = mxCreateString(str_value.c_str());
    }
    else if (value->IsArray())
    {
      v8::Local<v8::Array> jsArr = v8::Local<v8::Array>::Cast(info[1]);
      v8::Local<v8::Value> jsElement = jsArr->Get(0);
      int type = jsElement->IsBooleanObject() ? (jsElement->IsNumberObject() ? (jsElement->IsStringObject() ? 1 : 0) : 2) : 3; // 0-bad, 1-cell, 2-double, 3-logical
      if (!type)
        return Nan::ThrowTypeError("Unsupported variable type in the array");

      // decide the mxArray type
      int len = jsArr->Length();
      for (int i = 1; i < len; ++i)
      {
        jsElement = jsArr->Get(i);
        int elem_type = jsElement->IsBooleanObject() ? (jsElement->IsNumberObject() ? (jsElement->IsStringObject() ? 1 : 0) : 2) : 3; // 0-bad, 1-cell, 2-double, 3-logical
        if (!elem_type)
          return Nan::ThrowTypeError("Unsupported variable type in the array");
        if (type != elem_type) // must be a cell
          type = 1;
      }

      if (type == 2)
      {
        val = mxCreateDoubleMatrix(len, 1, mxREAL);
        double *p = mxGetPr(val);
        for (int i = 0; i < len; ++i)
        {
          jsElement = jsArr->Get(i);
          *(p++) = jsElement->NumberValue();
        }
      }
      else if (type == 3)
      {
        val = mxCreateLogicalMatrix(len, 1);
        bool *p = (bool *)mxGetData(val);
        for (int i = 0; i < len; ++i)
        {
          jsElement = jsArr->Get(i);
          *(p++) = jsElement->BooleanValue();
        }
      }
      else
      {
        val = mxCreateCellMatrix(len, 1);
        for (int i = 0; i < len; ++i)
        {
          jsElement = jsArr->Get(i);
          if (jsElement->IsBooleanObject())
            mxSetCell(val, i, mxCreateLogicalScalar(jsElement->BooleanValue()));
          else if (jsElement->IsNumberObject())
            mxSetCell(val, i, mxCreateDoubleScalar(jsElement->NumberValue()));
          else // if (jsElement->IsStringObject()
          {
            Nan::Utf8String utf8_value(jsElement);
            std::string str_value(*utf8_value, utf8_value.length());
            mxSetCell(val, i, mxCreateString(str_value.c_str()));
          }
        }
      }
    }
  }

  // run the MATLAB && return its return value to JS
  info.GetReturnValue().Set(Nan::New(engPutVariable(self->ep, name.c_str(), val)));
}

NAN_METHOD(MatlabEngine4NodeJS::ClearOutputBuffer)
{
  // https://community.risingstack.com/using-buffers-node-js-c-plus-plus/

  // expect 1 arguments
  if (info.Length() != 0)
    return Nan::ThrowError(Nan::New("Expected no argument").ToLocalChecked());

  // unwrap this MatlabEngine4NodeJS
  MatlabEngine4NodeJS *self = Nan::ObjectWrap::Unwrap<MatlabEngine4NodeJS>(info.This());

  // run the MATLAB && return its return value to JS
  info.GetReturnValue().Set(Nan::New(engOutputBuffer(self->ep, NULL, 0)));
}

NAN_METHOD(MatlabEngine4NodeJS::SetOutputBuffer)
{
  // https://community.risingstack.com/using-buffers-node-js-c-plus-plus/

  // expect 1 arguments
  if (info.Length() != 1)
    return Nan::ThrowError(Nan::New("Expected only 1 argument").ToLocalChecked());

  // unwrap this MatlabEngine4NodeJS
  MatlabEngine4NodeJS *self = Nan::ObjectWrap::Unwrap<MatlabEngine4NodeJS>(info.This());

  Nan::MaybeLocal<v8::Value> maybe_value(info[1]);
  if (maybe_value.IsEmpty())
  if (!node::Buffer::HasInstance(info[0]->ToObject()))
    return Nan::ThrowError(Nan::New("Input argument must be a node::Buffer instance").ToLocalChecked());

  char *buffer = node::Buffer::Data(info[0]);
  size_t size = node::Buffer::Length(info[0]);

  // run the MATLAB && return its return value to JS
  info.GetReturnValue().Set(Nan::New(engOutputBuffer(self->ep, buffer, (int)size)));
}
