#include <iostream>
#include <string>
#include <vector> 
#include "pipeline/pipeline.h"
#include <json-glib/json-glib.h>

JsonObject *load_json_config(const std::string &file_path,JsonParser** out_parser){
    GError *error=nullptr;
    JsonParser *parser=json_parser_new();
    if(!json_parser_load_from_file(parser,file_path.c_str(),&error)){
        std::cerr << "Error loading the config file "<<(error->message)<< std::endl;
        g_error_free(error);
        g_object_unref(parser);
        return nullptr;
    }
    *out_parser=parser;
    return json_node_get_object(json_parser_get_root(parser));
}

int main(int argc,char** argv){
    gst_init(&argc,&argv);
    JsonParser *parser=nullptr;
    JsonObject *root=load_json_config("configs/app_config.json",&parser);
    if(!root){
        return -1;
    }

    
    DeepstreamPipeline pipeline(root);
    pipeline.build();
    pipeline.run();
    g_object_unref(parser);
    return 0;
}