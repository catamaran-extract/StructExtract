#include <node.h>
#include <v8.h>

#include <string>
#include <vector>

#include "addon.h"

using namespace v8;

/*void ExtractStructFromString(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    v8::String::Utf8Value param(args[0]->ToString());
    std::string str(*param);
    
    std::vector<std::vector<std::string>> table_vec;
    std::string output_filename;
    ExtractStructFromString(str);
  
    Local<Array> table = Array::New(isolate);
    for (int i = 0; i < table_vec.size(); ++i) {
        Local<Array> list = Array::New(isolate);
        for (int j = 0; j < table_vec[i].size(); ++j)
            list->Set(j, String::NewFromUtf8(isolate, table_vec[i][j].c_str()));    
        table->Set(i, list);
    }

    args.GetReturnValue().Set(table);    
}

void ExtractStructFromFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    v8::String::Utf8Value param(args[0]->ToString());
    std::string str(*param);
    
    std::vector<std::vector<std::string>> table_vec;
    std::string output_filename;
    ExtractStructFromString(str);	
}*/

void NodeSelectSchema(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	
	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Hello World From SelectSchema!");
}

void NodeCandidateGen(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	
	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Hello World From CandidateGen!");
}

void NodeExtractFromFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    v8::String::Utf8Value param(args[0]->ToString());
	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Hello World From Extract!");
}

void Init(Handle<Object> exports, Handle<Object> module) {
	NODE_SET_METHOD(exports, "extract_from_file", NodeExtractFromFile);
	NODE_SET_METHOD(exports, "candidate_gen", NodeCandidateGen);
	NODE_SET_METHOD(exports, "select_schema", NodeSelectSchema);	
}

NODE_MODULE(catamaran, Init);