
#include <glib.h>
#include <json-glib/json-glib.h>
 
#include <iostream>

//helper function to write config file

static gboolean write_config_file(const gchar* filename,GString *content,GError **error){
    gboolean result=g_file_set_contents(filename,content->str,content->len,error);
    if(result){
        g_print("Config file created \n");
    }else{
        g_printerr("Failed file creatd file: %s\n",error && *error ?(*error)->message : "Unknown error");
    }
    return result;
}

void print_json_object(JsonObject *obj, const char *label) {
    // Create a node from the object
    JsonNode *node = json_node_new(JSON_NODE_OBJECT);
    json_node_set_object(node, obj);
    
    // Create generator
    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, node);
    
    
    // Generate string
    gchar *json_str = json_generator_to_data(generator, NULL);
    
    // Print
    std::cout << json_str << std::endl;
    
    // Cleanup
    g_free(json_str);
    g_object_unref(generator);
    json_node_free(node);
}
static void process_roi_filtering(JsonObject *config,GString *output,gint stream_index){
    if(!json_object_has_member(config,"roi-filtering-stream")){
        return ;
    }
    JsonObject *roi=json_object_get_object_member(config,"roi-filtering-stream");
    g_string_append_printf(output,"[roi-filtering-stream-%d]\n",stream_index);
    g_string_append(output,"enable=1\n");
    //Get all members 
    GList *members =json_object_get_members(roi);
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar*)l->data;
        //Handle ROI cordinateds
        if(g_str_has_prefix(key,"roi-")){
            const gchar *value=json_object_get_string_member(roi,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    //handle other parameters 
    if(json_object_has_member(roi,"inverse-roi")){
        gint64 inverse=json_object_get_int_member(roi,"inverse-roi");
        g_string_append_printf(output,"inverse-roi=%ld\n",inverse);
    }
    if(json_object_has_member(roi,"class-id")){
        gint64 class_id=json_object_get_int_member(roi,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}
static void process_line_crossing(JsonObject *config,GString *output,gint stream_index){
    if(!json_object_has_member(config,"line-crossing-stream")){
        return ;
    }
    JsonObject *lines=json_object_get_object_member(config,"line-crossing-stream");
    g_string_append_printf(output,"[line-crossing-stream-%d]\n",stream_index);
    g_string_append(output,"enable=1\n");
    //Get all members
    GList *members=json_object_get_members(lines);
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar*)l->data;
        //Handle line coordinates
        if(g_str_has_prefix(key,"line-crossing-")){
            const gchar* value=json_object_get_string_member(lines,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    //Handle other parameters
    if(json_object_has_member(lines,"class-id")){
        gint64 class_id=json_object_get_int_member(lines,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    if(json_object_has_member(lines,"extended")){
        gint64 extended=json_object_get_int_member(lines,"extended");
        g_string_append_printf(output,"extended=%ld\n",extended);
    }
    if(json_object_has_member(lines,"mode")){
        const gchar *mode=json_object_get_string_member(lines,"mode");
        g_string_append_printf(output,"mode=%s\n",mode);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}
static void process_overcrowding(JsonObject *config,GString *output,gint stream_index){
    
    if(!json_object_has_member(config,"overcrowding-stream")){
        return ;
    }
   ;
    JsonObject *crowd=json_object_get_object_member(config,"overcrowding-stream");
    g_string_append_printf(output,"[overcrowding-stream-%d]\n",stream_index);
    g_string_append(output,"enable=1\n");
    //Get all member
    GList *members=json_object_get_members(crowd);
     
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar *)l->data;
        
        //Handle ROI coordinates
        if(g_str_has_prefix(key,"roi-")){
            const char *value=json_object_get_string_member(crowd,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    
    
    //handle other parameter
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

static void process_direction_detection(JsonObject* config,GString *output,gint stream_index){
    if(!json_object_has_member(config,"direction-detection-stream")){
        return ;
    }

    JsonObject *dir=json_object_get_object_member(config,"direction-detection-stream");
    g_string_append_printf(output,"[direction-detection-stream-%d]\n",stream_index);
    g_string_append(output,"enable=1\n");
    GList *members=json_object_get_members(dir);
    for(GList *l=members;l!=NULL;l=l->next){
        const gchar *key=(const gchar *)l->data;
        //Handle direction coordinates
        if(g_str_has_prefix(key,"direction")){
            const gchar* value=json_object_get_string_member(dir,key);
            g_string_append_printf(output,"%s=%s\n",key,value);
        }
    }
    //Handle other paramete
    if(json_object_has_member(dir,"class-id")){
        gint64 class_id=json_object_get_int_member(dir,"class-id");
        g_string_append_printf(output,"class-id=%ld\n",class_id);
    }
    g_string_append(output,"\n");
    g_list_free(members);
}

gboolean parse_json_string_create_config(const gchar *json_string,const gchar *output_config,GError **error){

    JsonParser *parser=json_parser_new();
    if(!json_parser_load_from_data(parser,json_string,-1,error)){
        g_object_unref(parser);
        return FALSE;
    }
    JsonNode *root=json_parser_get_root(parser);
    if(!JSON_NODE_HOLDS_OBJECT(root)){
        g_set_error(error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"root element is not a json object");
        g_object_unref(parser);
        return FALSE;
    }
    JsonObject *root_obj=json_node_get_object(root);
    if(!json_object_has_member(root_obj,"data")){
        g_set_error(error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"json missing 'data' section");
        g_object_unref(parser);
        return FALSE;
    }
    JsonObject *data=json_object_get_object_member(root_obj,"data");
    
    //start building config file
    GString *config_content=g_string_new("");
    //add property
    g_string_append(config_content,"[property]\n");
    g_string_append(config_content,"enable=1\n");
    g_string_append(config_content,"config-width=1920\n");
    g_string_append(config_content,"config-height=1080\n");
    g_string_append(config_content,"osd-mode=2\n");
    g_string_append(config_content,"display-font-size=12\n");
    g_string_append(config_content,"\n");

    //process each schema
    GList *stream_ids=json_object_get_members(data);
    gint stream_index=0;
    gint active_stream=0;
    for(GList *l=stream_ids;l!=NULL;l=l->next){
        const gchar *stream_id=(const gchar *)l->data;
        JsonObject *stream=json_object_get_object_member(data,stream_id);
        if(!json_object_has_member(stream,"config")){
            std::cout <<"Skipping stream (no config member)" << stream_id << std::endl;
            continue;
        }
        JsonObject *config=json_object_get_object_member(stream,"config");
        if(json_object_get_size(config)==0){
            std::cout << "Skipping stream empty config "<< stream_id<< std::endl;
            continue;
        } 
        //add stream commit 
        g_string_append_printf(config_content,"## Stream %d - %s\n",stream_index,stream_id);
        //Process all analtyics sections
        process_roi_filtering(config,config_content,stream_index);
        
        process_line_crossing(config,config_content,stream_index);
        
        process_overcrowding(config,config_content,stream_index);
     
        process_direction_detection(config,config_content,stream_index);
        
        stream_index++;
        active_stream++;
    }
    g_list_free(stream_ids);

    // Write to file
    gboolean result=write_config_file(output_config,config_content,error);
    g_string_free(config_content,TRUE);
    g_object_unref(parser);
    return result;
}

gboolean json_glib_create_config(const gchar *json_file,const gchar *output_config,GError **error){
    JsonParser *parser=json_parser_new();
    
    if(!json_parser_load_from_file(parser, json_file,error)){
        g_object_unref(parser);
        return FALSE;
    }
    
    JsonNode *root=json_parser_get_root(parser);
    JsonGenerator *gen=json_generator_new();
    json_generator_set_root(gen,root);
    gchar *json_string=json_generator_to_data(gen,NULL);
    g_object_unref(gen);
    g_object_unref(parser);
    
    gboolean result=parse_json_string_create_config(json_string,output_config,error);
    
    g_free(json_string);
    return result;

}
