#include "analyticsconfigwriter.h"
#include <iostream>
#include <fstream>

AnalyticsConfigWriter::AnalyticsConfigWriter():m_configWidth(1920),m_configHeight(1080),m_osdMode(2){

}
AnalyticsConfigWriter::~AnalyticsConfigWriter(){

}

void AnalyticsConfigWriter::setResolution(int width,int height){
    m_configWidth=width;
    m_configHeight=height;

}
void AnalyticsConfigWriter::setOsdMode(int mode){
    m_osdMode=mode;
}
bool AnalyticsConfigWriter::writeConfigFile(const std::string &filename,GString *content){
    std::ofstream outFile(filename);
    if(!outFile.is_open()){
        std::cerr << "Failed to create config file "<< filename << std::endl;
        return false;
    }
    outFile.write(content->str,content->len);
    outFile.close();
    if(outFile.good()){
        std::cout << "Config file create successfully " << filename << std::endl;
        return true;
    }else{
        std::cerr << "Error writing to config file "<< filename << std::endl;
        return false;
    }

}
 
void AnalyticsConfigWriter::printJsonObject(JsonObject *obj){
    JsonNode *node =json_node_new(JSON_NODE_OBJECT);
    json_node_set_object(node,obj);

    JsonGenerator *generator =json_generator_new();
    json_generator_set_root(generator,node);
    gchar *json_str=json_generator_to_data(generator,NULL);
    std::cout << "DEBUG JSON: "<< json_str << std::endl;
    g_free(json_str);
    g_object_unref(generator);
    json_node_free(node);
}
//process logic

void AnalyticsConfigWriter::processRoiFiltering(JsonObject *config,GString *output,int streamIndex){
    if(!json_object_has_member(config,"roi-filtering-stream")){
        return ;
    }
    JsonObject *roi=json_object_get_object_member(config,"roi-filtering-stream");
    g_string_append_printf(output,"[roi-filtering-stream-%d]\n",streamIndex);
    g_string_append(output,"enable=1\n");
    
    GList *members=json_object_get_members(roi);
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar* )l->data;
        if(g_str_has_prefix(key,"roi-")){
            const gchar *value=json_object_get_string_member(roi,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    if(json_object_has_member(roi,"inverse-roi")){
        gint64 class_id=json_object_get_int_member(roi,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}
void AnalyticsConfigWriter::processLineCrossing(JsonObject *config,GString *output,int streamIndex){
    if(!json_object_has_member(config,"line-crossing-stream")){
        return ;
    }
    JsonObject *lines=json_object_get_object_member(config,"line-crossing-stream");
    g_string_append_printf(output,"[line-crossing-stream-%d]\n",streamIndex);   
    g_string_append(output,"enable=1\n");

    GList *members=json_object_get_members(lines);
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar* )l->data;
        if(g_str_has_prefix(key,"line-crossing-")){
            const gchar* value=json_object_get_string_member(lines,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    if(json_object_has_member(lines,"class-id")){
        gint64 class_id=json_object_get_int_member(lines,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    if(json_object_has_member(lines,"extended")){
        gint64 extended =json_object_get_int_member(lines,"extended");
        g_string_append_printf(output,"extended=%ld\n",extended);
    }
    if(json_object_has_member(lines,"mode")){
        const gchar *mode=json_object_get_string_member(lines,"mode");
        g_string_append_printf(output,"mode=%s\n",mode);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}
void AnalyticsConfigWriter::processOvercrowding(JsonObject *config,GString *output,int streamIndex){
    if(!json_object_has_member(config,"overcrowding-stream")){
        return;
    }
    JsonObject *crowd=json_object_get_object_member(config,"overcrowding-stream");
    g_string_append_printf(output,"[overcrowding-stream-%d]\n",streamIndex);
    g_string_append(output,"enable=1\n");
    GList *members=json_object_get_members(crowd);
    for(GList *l=members;l!=NULL;l=l->next){
        const char *key =(const gchar *)l->data;
        if(g_str_has_prefix(key,"roi-")){
            const char *value=json_object_get_string_member(crowd,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    if(json_object_has_member(crowd,"object-threshold")){
        gint64 threshold=json_object_get_int_member(crowd,"object-threshold");
        g_string_append_printf(output,"object-threshold=%ld\n",threshold);
    }
    if(json_object_has_member(crowd,"class-id")){
        gint64 class_id=json_object_get_int_member(crowd,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}
void AnalyticsConfigWriter::processDirectionDetection(JsonObject *config,GString *output,int streamIndex){
    if(!json_object_has_member(config,"direction-detection-stream")){
        return ;
    }
    JsonObject *dir=json_object_get_object_member(config,"direction-detection-stream");
    g_string_append_printf(output,"[direction-detection-stream-%d]\n",streamIndex);
    g_string_append(output,"enable=1\n");
    
    GList *members=json_object_get_members(dir);
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar *)l->data;
        if(g_str_has_prefix(key,"direction")){
            const gchar* value=json_object_get_string_member(dir,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    if(json_object_has_member(dir,"class-id")){
        gint64 class_id=json_object_get_int_member(dir,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}
bool AnalyticsConfigWriter::generateFromString(const std::string &jsonconfig,const std::string &outputConfigPath){
    GError *error=nullptr;
    JsonParser *parser=json_parser_new();
    if(!json_parser_load_from_data(parser,jsonconfig.c_str(),-1,&error)){
        std::cerr << "JSON Parser Error: "<< (error ? error->message:"Unknown")<< std::endl;
        g_object_unref(parser);
        return false;
    }
    JsonNode *root=json_parser_get_root(parser);
    if(!JSON_NODE_HOLDS_OBJECT(root)){
        std::cerr << "Error : Root element is not a JSON object "<< std::endl;
        g_object_unref(parser);
        return false;
    }
    JsonObject *rootObj=json_node_get_object(root);
    if(!json_object_has_member(rootObj,"data")){
        std::cerr << "Error : JSON missing 'data' section "<< std::endl;
        g_object_unref(parser);
        return false;
    }
    JsonObject *data=json_object_get_object_member(rootObj,"data");
    
    //  Build Config Content
    GString *configContent=g_string_new("");

    //Add property Header
    g_string_append(configContent,"[property]\n");
    g_string_append(configContent,"enable=1\n");
    g_string_append_printf(configContent,"config-width=%d\n",m_configWidth);
    g_string_append_printf(configContent,"config-height=%d\n",m_configHeight);
    g_string_append_printf(configContent,"osd-mode=%d\n",m_osdMode);
    g_string_append(configContent,"display-font-size=12\n");
    g_string_append(configContent,"\n");

    //Iterate streams
    GList *streamIds =json_object_get_members(data);
    int streamIndex=0;
    for(GList *l=streamIds;l!=NULL;l=l->next){
        const gchar *streamId=(const gchar *)l->data;
        if(!json_object_has_member(data,streamId)){
            continue;//safety check
        }
        JsonObject *stream=json_object_get_object_member(data,streamId);
        if(!json_object_has_member(stream,"config")){
            std::cout << "Skipping stream(no config member ): "<< streamId << std::endl;
            continue;
        }
        JsonObject *config=json_object_get_object_member(stream,"config");
        if(json_object_get_size(config)==0){
            std::cout << "Skipping stream empty config: "<< streamId << std::endl;
            continue;
        }
        g_string_append_printf(configContent,"## Stream %d - %s\n",streamIndex,streamId);
        processRoiFiltering(config,configContent,streamIndex);
        processLineCrossing(config,configContent,streamIndex);
        processOvercrowding(config,configContent,streamIndex);
        processDirectionDetection(config,configContent,streamIndex);
        streamIndex++;
    }
    g_list_free(streamIds);
    //writing to file
    bool result=writeConfigFile(outputConfigPath,configContent);

    //cleanup
    g_string_free(configContent,TRUE);
    g_object_unref(parser);
    return result;
}
 
 
bool AnalyticsConfigWriter::generateFromFile(const std::string &jsonInputPath,const std::string &outputConfigPath){
    GError *error=nullptr;
    JsonParser *parser=json_parser_new();

    if(!json_parser_load_from_file(parser,jsonInputPath.c_str(),&error)){
        std::cerr << "Failed to load json file : "<< (error ? error->message :"unknown")<< std::endl;
        g_object_unref(parser);
        if(error){
            g_error_free(error);
            
        }
        return false;
    }
    JsonNode *root=json_parser_get_root(parser);
    JsonGenerator *gen=json_generator_new();
    json_generator_set_root(gen,root);
    gchar *jsonstr=json_generator_to_data(gen,NULL);
    g_object_unref(gen);
    g_object_unref(parser);

    bool result=generateFromString(std::string(jsonstr),outputConfigPath);
    g_free(jsonstr);
    return result;
}
