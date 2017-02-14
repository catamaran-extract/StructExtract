#include <node.h>
#include <v8.h>

using namespace v8;

void ExtractStructFromString(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    v8::String::Utf8Value param(args[0]->ToString());
    std::string str(*param);
    
    ExtractStructFromString(str);
}

void GetRow(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();    
    
    int index = args[0]->NumberValue();
    std::vector<std::string> row;
    GetRow(index, &row);
}

void Init(Handle<Object> exports, Handle<Object> module) {
    NODE_SET_METHOD(exports, "extract_struct_from_string", ExtractStructFromString);
    NODE_SET_METHOD(exports, "get_tuple", GetTuple);
}

NODE_MODULE(catamaran, Init);