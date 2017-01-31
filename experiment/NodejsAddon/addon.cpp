#include <node.h>
#include <v8.h>

using namespace v8;

void DemoFunction(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    Local<v8::String> retval = v8::String::NewFromUtf8(isolate, "Hello World!");
    args.GetReturnValue().Set(retval);
}

void init(Handle<Object> exports, Handle<Object> module) {
    NODE_SET_METHOD(exports, "demo", DemoFunction);
}

NODE_MODULE(catamaran, init);