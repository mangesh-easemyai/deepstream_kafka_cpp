#include "pipeline.h"

DeepstreamPipeline::DeepstreamPipeline(JsonObject *config_root,guint rtsp_port,guint udp_port,std::string infer_config_path,std::string tracker_config_path,std::string analytics_config_path):config_root_(config_root),rtsp_port_(rtsp_port),udp_port_(udp_port),infer_config_path_(infer_config_path),tracker_config_path_( tracker_config_path),analytics_config_path_(analytics_config_path){
    
    
    if(config_root){
        if(json_object_has_member(config_root,"environment")){
            const gchar *env=json_object_get_string_member(config_root,"environment");
            std::cout << "Environment: "<< env << std::endl;
        }
    }
    if(config_root && json_object_has_member(config_root,"data")){
        JsonObject *data_obj=json_object_get_object_member(config_root,"data");
        JsonObjectIter iter;
        const gchar *key;
        JsonNode *node;
        json_object_iter_init(&iter,data_obj);
        while(json_object_iter_next(&iter,&key,&node)){
            JsonObject *stream_obj=json_node_get_object(node);
            if(stream_obj && json_object_has_member(stream_obj,"link")){
                const gchar *link=json_object_get_string_member(stream_obj,"link");
                urls_.push_back(std::string(link));
                std::cout << "Addesd URL "<< link << std::endl;
            }
        }
    }
    std::cout << "Pipeline Initialized with "<< urls_.size() << " sources for tesing "<< std::endl;

}
DeepstreamPipeline::~DeepstreamPipeline(){
 stop();
}

void DeepstreamPipeline::build(){
    gst_init(nullptr,nullptr);
    loop_=g_main_loop_new(nullptr,FALSE);
    pipeline_=gst_pipeline_new("test-pipeline");
    streammux=gst_element_factory_make("nvstreammux","stream-muxer");
    primary_nvinference_=gst_element_factory_make("nvinferserver","primary-nvinference-enginer");
    nvtracker_=gst_element_factory_make("nvtracker","nvtracker");
    nvdsanalytics_=gst_element_factory_make("nvdsanalytics","nvdsanalytics");
    tee_=gst_element_factory_make("tee","tee");
    nvosd_=gst_element_factory_make("nvdsosd","nv-onscreendisplay");
    tiler_=gst_element_factory_make("nvmultistreamtiler","nvtiler");
    encoder_=gst_element_factory_make("nvv4l2h264enc","h264-encoder");
    parse_=gst_element_factory_make("h264parse","h264-parse");
    payloader_=gst_element_factory_make("rtph264pay","rtp-payer");
    udpsink_=gst_element_factory_make("udpsink","udp-sink");

    queue_encoder_=gst_element_factory_make("queue","queue_encoder");
    queue_infer_=gst_element_factory_make("queue","queue_infer");
    queue_osd_=gst_element_factory_make("queue","queue_osd");
    queue_display_ = gst_element_factory_make("queue","queue_display");
    queue_parse_=gst_element_factory_make("queue","parse_encoder");
    queue_payloader_=gst_element_factory_make("queue","queue_payloader");
    queue_tiler_=gst_element_factory_make("queue","queue_nvtiler");

    //kafka branch
    queue_kafka_=gst_element_factory_make("queue","queue_kafka");
    nvmsgconv_=gst_element_factory_make("nvmsgconv","nvmsgconv");
    nvmsgbroker_=gst_element_factory_make("nvmsgbroker","nvmsgbroker");

    if(!pipeline_ ||!streammux|| !primary_nvinference_|| !nvtracker_||!nvdsanalytics_|| !tee_|| !queue_tiler_ ||!tiler_ ||!queue_osd_ ||!nvosd_||!queue_encoder_||!encoder_ ||!queue_parse_ ||!parse_||!queue_payloader_ ||!payloader_||!udpsink_){
        std::cerr << "Build Error: failed to create element" << std::endl;
        return;
    }
    

    int batch_size=urls_.size();
    if(batch_size>4){
        muxer_width_=640;
        muxer_height_=360;
        std::cout << "High source count detected. Switching to low-res tracking: " 
                  << muxer_width_ << "x" << muxer_height_ << std::endl;
    }else{
        muxer_width_=1920;
        muxer_height_=1080;
    }
    
    g_object_set(streammux,"batch-size",batch_size,"width",muxer_width_,"height",muxer_height_,"batched-push-timeout",40000,
                "enable-padding",TRUE,nullptr);
    guint tiler_rows=(guint)ceil(sqrt(batch_size));
    guint tiler_cols=(guint)ceil((double)batch_size/tiler_rows);
    AnalyticsConfigWriter analyticsWriter;
    analyticsWriter.setResolution(muxer_width_,muxer_height_);
    JsonGenerator *gen=json_generator_new();
    JsonNode *root_node=json_node_new(JSON_NODE_OBJECT);
    json_node_set_object(root_node,config_root_);
    json_generator_set_root(gen,root_node);
    gchar *json_str=json_generator_to_data(gen,NULL);
    if(!analyticsWriter.generateFromString(std::string(json_str),analytics_config_path_)){
        std::cerr << "Warning : Failed to generate analytics config file "<< std::endl;

    }
    g_free(json_str);
    json_node_free(root_node);
    g_object_unref(gen);

    g_object_set(primary_nvinference_,"config-file-path",infer_config_path_.c_str(),nullptr);
    g_object_set(nvdsanalytics_,"config-file",analytics_config_path_.c_str(),nullptr);
    if(!set_tracker_properties(nvtracker_)){
        std::cerr << "FATAL : Failed to configure tracker, check config path" << tracker_config_path_ << std::endl;
        return;
    }

    g_object_set(nvosd_,"process-mode",1,nullptr);
    g_object_set(tiler_,"rows",tiler_rows,"columns",tiler_cols,"width",1920,"height",1080,nullptr);
    
    g_object_set(encoder_,"bitrate",4000000,
                 
                "profile",0,nullptr);
    g_object_set(encoder_,"insert-sps-pps",1,"iframeinterval",30,"idrinterval",30,nullptr);
    g_object_set(payloader_,"config-interval",0,"pt",96,nullptr);
    
    g_object_set(nvmsgconv_,"config","configs/msgconv_config.txt","payload-type",0, "msg2p-newapi",1,nullptr);
    g_object_set(nvmsgbroker_,"proto-lib","configs/libnvds_kafka_proto.so","conn-str","kafka;9092","topic","deepstream-analytics","sync",false,nullptr);
    g_object_set(queue_infer_,"max-size-buffers",5,nullptr);
    g_object_set(queue_osd_,"max-size-buffers",5,nullptr);
    g_object_set(queue_tiler_,"max-size-buffers",5,nullptr);
    g_object_set(queue_encoder_,"max-size-buffers",5,nullptr);
    g_object_set(queue_parse_,"max-size-buffers",5,nullptr);
    g_object_set(queue_payloader_,"max-size-buffers",5,nullptr);
    g_object_set(udpsink_,"host","127.0.0.1",
                "port",udp_port_,
                "sync",TRUE,
                "async",FALSE,nullptr);
 
    gst_bin_add_many(GST_BIN(pipeline_),streammux, queue_infer_, primary_nvinference_, nvtracker_, nvdsanalytics_,
    tee_,
    //display branch
    queue_display_,queue_tiler_,tiler_,queue_osd_,nvosd_,queue_encoder_,encoder_,queue_parse_,parse_,queue_payloader_,payloader_,udpsink_,
    //kafka branch
    queue_kafka_,nvmsgconv_,nvmsgbroker_,
    nullptr);

    if(!gst_element_link_many(streammux,   queue_infer_, primary_nvinference_,nvtracker_,nvdsanalytics_,tee_,nullptr)){
        std::cerr << "Build Error: Failed to link main pipeline" << std::endl;
        return;
    }
    GstPad *tee_disp_pad=gst_element_request_pad_simple(tee_,"src_%u");
    GstPad *q_disp_pad=gst_element_get_static_pad(queue_display_,"sink");

    if(gst_pad_link(tee_disp_pad,q_disp_pad)!=GST_PAD_LINK_OK){
        std::cerr << "FATAL: Failed to link display branch "<< std::endl;
        return ;
    }
    gst_object_unref(tee_disp_pad);
    gst_object_unref(q_disp_pad);

    GstPad *tee_kafka_pad=gst_element_request_pad_simple(tee_,"src_%u");
    GstPad *q_kafka_pad=gst_element_get_static_pad(queue_kafka_,"sink");
    if(gst_pad_link(tee_kafka_pad,q_kafka_pad)!= GST_PAD_LINK_OK){
        std::cerr << "FATAL: Failed to link kafka branch "<< std::endl;
        return;
    }

    gst_object_unref(tee_kafka_pad);
    gst_object_unref(q_kafka_pad);

    if(!gst_element_link_many(queue_display_,tiler_,queue_osd_,nvosd_,queue_encoder_,encoder_,queue_parse_,parse_,queue_payloader_,payloader_,udpsink_,nullptr)){
        std::cerr << "Build Error: Failed to link display branch" << std::endl;
        return;
    }
    if(!gst_element_link_many(queue_kafka_,nvmsgconv_,nvmsgbroker_,nullptr)){
        std::cerr << "Build Error: Failed to link kafka branch" << std::endl;
        return;
    }

    for(int i=0; i< batch_size; i++){
        std::string elem_name = "source_" + std::to_string(i);
        GstElement *uri_decode_bin = gst_element_factory_make("nvurisrcbin", elem_name.c_str());
        
        if(!uri_decode_bin){
            std::cerr << "Build : Failed to create source element " << i << std::endl;
            return;
        }

        g_object_set(G_OBJECT(uri_decode_bin), 
                     "uri", urls_[i].c_str(),
                     "select-rtp-protocol", 4, 
                     "num-extra-surfaces", 4,
                     "latency", 200, 
                     "rtsp-reconnect-interval", 10, 
                     "rtsp-reconnect-attempts", -1, 
                     "file-loop", TRUE, 
                     nullptr);

        gst_bin_add(GST_BIN(pipeline_), uri_decode_bin);
        
        // Pass the index to the callback via data
        g_object_set_data(G_OBJECT(uri_decode_bin), "sink-index", GINT_TO_POINTER(i));
        
        // Connect to on_pad_added. Pass 'streammux' as user data
        g_signal_connect(G_OBJECT(uri_decode_bin), "pad-added", G_CALLBACK(on_pad_added), streammux);
        
        // Sync state
        gst_element_sync_state_with_parent(uri_decode_bin);
    }
    bus_=gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    bus_watch_id=gst_bus_add_watch(bus_,bus_call,loop_);
    gst_object_unref(bus_);
    if(!setup_rtsp_server()){
        std::cerr << "Failed to start RTSP server "<< std::endl;
        return;
    }
    NvDsObjEncCtxHandle obj_ctx_handle=nvds_obj_enc_create_context(gpu_id);
    if(!obj_ctx_handle){
        std::cerr << "Unable to create context"<< std::endl;
        return ;
    }
    ProbeData *probe_data=new ProbeData();
    probe_data->obj_ctx_handle=obj_ctx_handle;
    probe_data->root_obj=config_root_;
    GstPad *analytics_src_pad=gst_element_get_static_pad(nvdsanalytics_,"src");
    if(!analytics_src_pad){
        std::cerr << "Failed to get src pad of nvanalytics src pad "<< std::endl;
        return;
    }
    if(config_root_ && json_object_has_member(config_root_,"data")){
        JsonObject *data_obj=json_object_get_object_member(config_root_,"data");
        JsonObjectIter iter;
        const gchar *key;
        JsonNode *node;
        json_object_iter_init(&iter,data_obj);
        while(json_object_iter_next(&iter,&key,&node)){
            probe_data->source_ids.push_back(key);
        }
    }
    gst_pad_add_probe(analytics_src_pad,GST_PAD_PROBE_TYPE_BUFFER,nvdsanalytics_src_pad_buffer_probe,probe_data,nullptr);
    gst_object_unref(analytics_src_pad);
    std::cout << "Build Test Pipeline build succefully "<< std::endl;  
}

bool DeepstreamPipeline::set_tracker_properties(GstElement *nvtracker){
    GKeyFile *key_file =g_key_file_new();
    GError *error =nullptr;
    bool ret=false;

    const gchar* CONFIG_GROUP_TRACKER="tracker";
    const gchar* CONFIG_GROUP_TRACKER_WIDTH="tracker-width";
    const gchar* CONFIG_GROUP_TRACKER_HEIGHT="tracker-height";
    const gchar* CONFIG_GPU_ID="gpu-id";
    const gchar* CONFIG_GROUP_TRACKER_LL_CONFIG_FILE="ll-config-file";
    const gchar* CONFIG_GROUP_TRACKER_LL_LIB_FILE="ll-lib-file";
    
    if(!g_key_file_load_from_file(key_file,tracker_config_path_.c_str(),G_KEY_FILE_NONE,&error)){
        if(error){
            std::cerr << "Failed to load tracker config file : "<< tracker_config_path_ << std::endl;
            std::cerr <<"Error : "<< error->message << std::endl;
            g_error_free(error);
        }else{
            std::cerr << "Failed to load tracker config file (unknown error )"<< std::endl;
        }
        g_key_file_free(key_file);
        return false;
    }

    gsize num_keys=0;
    gchar **keys =g_key_file_get_keys(key_file,CONFIG_GROUP_TRACKER,&num_keys,&error);
    if(error){
        std::cerr << "Error reading keys from tracker config "<< error->message << std::endl;
        g_error_free(error);
        goto done;
    }
    g_object_set(G_OBJECT(nvtracker),"tracker-height",muxer_height_,
                "tracker-width",muxer_width_,nullptr);

    for(gsize i=0;i<num_keys;i++){
        gchar *key=keys[i];
        
        if(!g_strcmp0(key,CONFIG_GPU_ID)){
            guint gpu_id=g_key_file_get_integer(key_file,CONFIG_GROUP_TRACKER,CONFIG_GPU_ID,&error);
            if(!error){
                g_object_set(G_OBJECT(nvtracker),"gpu-id",gpu_id,nullptr);
                 
            }
            else{
                g_error_free(error);
                error=nullptr;
            }
        }
        else if(!g_strcmp0(key,CONFIG_GROUP_TRACKER_LL_CONFIG_FILE)){
            gchar *ll_config_path_str=g_key_file_get_string(key_file,CONFIG_GROUP_TRACKER,CONFIG_GROUP_TRACKER_LL_CONFIG_FILE,&error);
            if(!error){
                std::string abs_path=get_absolute_file_path(tracker_config_path_,ll_config_path_str);
                std::cout << "Setting Tracker LL config:  "<< abs_path << std::endl;
                g_object_set(G_OBJECT(nvtracker),"ll-config-file",abs_path.c_str(),nullptr);
            }
        }else if(!g_strcmp0(key,CONFIG_GROUP_TRACKER_LL_LIB_FILE)){
            gchar *ll_lib_path_str=g_key_file_get_string(key_file,CONFIG_GROUP_TRACKER,CONFIG_GROUP_TRACKER_LL_LIB_FILE,&error);
            if(!error){
                std::string abs_path=get_absolute_file_path(tracker_config_path_,ll_lib_path_str);
                std::cout << "Setting Tracker ll lib : "<< abs_path << std::endl;
                g_object_set(G_OBJECT(nvtracker),"ll-lib-file",abs_path.c_str(),nullptr);
                g_free(ll_lib_path_str);
            }
        }else{
            std::cerr << "Unknown key" << key << " in group [ "<<CONFIG_GROUP_TRACKER <<  "]" << std::endl;
        }
        if(error){
            std::cerr << "Error parsing key "<< key << ": "<< error->message << std::endl;
            g_error_free(error);
            error=nullptr;
        }
    }
    ret=true;
done:
    if(keys){
        g_strfreev(keys);
    }
    if(key_file){
        g_key_file_free(key_file);
    }
    if(error){
        g_error_free(error);
    }
    return ret;
}


gboolean DeepstreamPipeline::setup_rtsp_server(){
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    std::string udpsrc_pipeline;
    std::string port_num_str;
    guint64 udp_buffer_size = 4 * 1024 * 1024;
    udpsrc_pipeline ="( udpsrc name=pay0 port=" + std::to_string(udp_port_) +
" buffer-size="+std::to_string(udp_buffer_size)+
" caps=\"application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96\" )";

                    
    server_=gst_rtsp_server_new();
    g_object_set(server_,"service",std::to_string(rtsp_port_).c_str(),nullptr);
    mounts=gst_rtsp_server_get_mount_points(server_);
    factory=gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory,udpsrc_pipeline.c_str());
    std::string mount_path="/"+service_id_;
    gst_rtsp_mount_points_add_factory(mounts,mount_path.c_str(),factory);
    g_object_unref(mounts);
    if(gst_rtsp_server_attach(server_,nullptr)==0){
        std::cerr<<"Failed to attach RTSP server  " << std::endl;
        return FALSE;
    }
    std::cout << "\n *** Deepstream Launched RTSP streamming at rtsp://localhost:"<<rtsp_port_ << "/"<<service_id_<< "***\n";
    return TRUE;
}

void DeepstreamPipeline::run(){
    if(pipeline_){
        std::cout << "RUN Starting test pipeline... " << std::endl;
        gst_element_set_state(pipeline_,GST_STATE_PLAYING);
        g_main_loop_run(loop_);
    }
}

void DeepstreamPipeline::stop(){
    if(pipeline_){
        std::cout << "STOP stopping pipeline"<< std::endl;
        gst_element_set_state(pipeline_,GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline_));
        pipeline_=nullptr;
    }
    if(loop_){
        g_main_loop_unref(loop_);
        loop_=nullptr;
    }
    if(server_){
        g_object_unref(server_);
        server_=nullptr;
    }
    source_bins_.clear();
}
GstElement* DeepstreamPipeline::create_source_bin(guint index,const std::string &uri){
    GstElement *bin =nullptr,*uri_decode_bin=nullptr;
    std::string bin_name="source_bin-"+std::to_string(index);
    bin=gst_bin_new(bin_name.c_str());
    uri_decode_bin=gst_element_factory_make("nvurisrcbin","uri-decode-bin");
    // g_object_set(G_OBJECT(uri_decode_bin),"rtsp-reconnect-interval",10,
    //             "rtsp-reconnect-attempts",-1,"latency",100,nullptr);
    g_object_set(G_OBJECT(uri_decode_bin), 
                 "select-rtp-protocol", 4,       // <--- 4 = rtp-tcp (The correct way to force TCP)
                 "num-extra-surfaces", 4,        // <--- Valid
                 "latency", 200,                  // <--- Valid (Increased for stability)
                 "rtsp-reconnect-interval", 10,   // <--- Valid
                 "rtsp-reconnect-attempts", -1,   // <--- Valid
                 "file-loop", TRUE,               // <--- Valid
                 nullptr);
    if(!bin || !uri_decode_bin){
        std::cerr << "One element in source bin could not be created "<< std::endl;
        return nullptr;
    }
    g_object_set(G_OBJECT(uri_decode_bin),"uri",uri.c_str(),nullptr);
    g_signal_connect(G_OBJECT(uri_decode_bin),"pad-added",G_CALLBACK(cb_newpad),bin);
    g_signal_connect(G_OBJECT(uri_decode_bin),"child-added",G_CALLBACK(decodebin_child_added),bin);
    gst_bin_add(GST_BIN(bin),uri_decode_bin);

    GstPad *ghost_pad=gst_ghost_pad_new_no_target("src",GST_PAD_SRC);

    if(!gst_element_add_pad(bin,ghost_pad)){
        std::cerr << "Failed to add ghost pad in source bin "<< std::endl;
        return nullptr;
    }
    return bin;
}
void DeepstreamPipeline::cb_newpad(GstElement *decodebin,GstPad *decoder_src_pad,gpointer data){
    GstElement *bin=GST_ELEMENT(data);
    GstCaps *caps=gst_pad_query_caps(decoder_src_pad,nullptr);
    if(!caps){
        return;
    }
    const GstStructure *str=gst_caps_get_structure(caps,0);
    const gchar *name=gst_structure_get_name(str);
    if(strncmp(name,"video",5)==0){
        GstPad *ghost_pad=gst_element_get_static_pad(bin,"src");
        if(!ghost_pad){
            std::cerr << "cb_newpad: Failed to get ghost pad "<< std::endl;
            gst_caps_unref(caps);
            return;
        }
        if(gst_ghost_pad_set_target(GST_GHOST_PAD(ghost_pad),decoder_src_pad)){
                std::cout << "Ghost pad target set successfully" << std::endl;
        }else{
            std::cerr << "cb_newpad : Failed to link decoder to ghost pad " << std::endl;

        }
        gst_object_unref(ghost_pad);
    }
    gst_caps_unref(caps);
}
void DeepstreamPipeline::on_pad_added(GstElement *src,GstPad *pad,gpointer data){
    GstElement *muxer=GST_ELEMENT(data);
    gint sink_index=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(src),"sink-index"));
    gchar *pad_name=g_strdup_printf("sink_%d",sink_index);
    GstPad *muxer_sink_pad=gst_element_request_pad_simple(muxer,pad_name);
    if(gst_pad_link(pad,muxer_sink_pad)==GST_PAD_LINK_OK){
        std::cout << "LINK Source " << sink_index << " Linked to Muxer "<< std::endl;
    }else{
        std::cerr  << "LINK Failed to link source "<< sink_index << std::endl;
    }
    g_free(pad_name);
    gst_object_unref(muxer_sink_pad);
}

gboolean DeepstreamPipeline::bus_call(GstBus *bus,GstMessage *msg,gpointer data){
    GMainLoop *loop=(GMainLoop * )data;
    switch(GST_MESSAGE_TYPE(msg)){
        case GST_MESSAGE_EOS:
            std::cout << "BUS EOS received "<< std::endl;
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:{
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg,&error,&debug);
            std::cerr << "BUS ERROR : " << error->message << std::endl;
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        case GST_MESSAGE_WARNING:{
            gchar *debug;
            GError *error;
            gst_message_parse_warning(msg,&error,&debug);
            std::cerr << "BUS WARNING : "<< error->message << std::endl;
            g_free(debug);
            g_error_free(error);
            break;
        }
        default:
            break;
    }
    return TRUE;
}
void DeepstreamPipeline::decodebin_child_added(GstChildProxy *child_proxy,GObject *object ,gchar *name,gpointer user_data){
    std::cout << "Decodebin child added "<< std::endl;
    if(g_strrstr(name,"decodebin")==name){
        g_signal_connect(G_OBJECT(object),"child-added",G_CALLBACK(decodebin_child_added),user_data);
    }
}