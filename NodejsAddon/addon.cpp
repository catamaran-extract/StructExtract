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

void NodeCandidateGen(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    v8::String::Utf8Value param1(args[0]->ToString());
    std::string filename(*param1);
    
    // Candidate Gen
    std::vector<std::string> vec, escaped;
    CandidateGenerate(filename, &vec, &escaped);
    
    Local<Array> schema_list = Array::New(isolate); 
    for (int i = 0; i < vec.size(); ++i)
        schema_list->Set(i, String::NewFromUtf8(isolate, vec[i].c_str()));
    
    Local<Array> escaped_list = Array::New(isolate);
    for (int i = 0; i < escaped.size(); ++i)
        escaped_list->Set(i, String::NewFromUtf8(isolate, escaped[i].c_str()));

    Local<Object> obj = Object::New(isolate);
    obj->Set(String::NewFromUtf8(isolate, "schema_list"), schema_list);
    obj->Set(String::NewFromUtf8(isolate, "escaped_list"), escaped_list);
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Hello World From CandidateGen!"));
}

void NodeSelectSchema(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    v8::String::Utf8Value param1(args[0]->ToString());
    std::string filename(*param1);
    
    std::vector<std::string> schema_list;
    Handle<Array> param2 = Handle<Array>::Cast(args[1]);
    for (int i = 0; i < param2->Length(); ++i) {
        v8::String::Utf8Value item(param2->Get(i)->ToString()); 
        std::string schema(*item);
        schema_list.push_back(schema);
    }
    
    // Select Schema
    std::string schema, escaped;
    SelectSchema(filename, schema_list, &schema, &escaped);
    
    Local<Object> obj = Object::New(isolate);
    obj->Set(String::NewFromUtf8(isolate, "schema"), String::NewFromUtf8(isolate, schema.c_str()));
    obj->Set(String::NewFromUtf8(isolate, "escaped"), String::NewFromUtf8(isolate, escaped.c_str()));   

    args.GetReturnValue().Set(obj);
}

void NodeExtractFromFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    v8::String::Utf8Value param1(args[0]->ToString());
    std::string filename(*param1);
    
    v8::String::Utf8Value param2(args[1]->ToString());
    std::string schema(*param2);
    
    std::vector<std::vector<std::string>> table_vec;
    // Extract
    ExtractFromFile(filename, schema, &table_vec);

    Local<Array> table = Array::New(isolate);
    for (int i = 0; i < table_vec.size(); ++i) {
        Local<Array> list = Array::New(isolate);
        for (int j = 0; j < table_vec[i].size(); ++j)
            list->Set(j, String::NewFromUtf8(isolate, table_vec[i][j].c_str()));    
        table->Set(i, list);
    }

    args.GetReturnValue().Set(table);
}

void Init(Handle<Object> exports, Handle<Object> module) {
    NODE_SET_METHOD(exports, "extract_from_file", NodeExtractFromFile);
    NODE_SET_METHOD(exports, "candidate_gen", NodeCandidateGen);
    NODE_SET_METHOD(exports, "select_schema", NodeSelectSchema);    
}

NODE_MODULE(catamaran, Init);