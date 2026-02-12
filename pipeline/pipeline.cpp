#include "pipeline.h"

DeepstreamPipeline::DeepstreamPipeline(std::vector<std::string> urls):urls(urls){
    std::cout << "Pipeline Initialized with "<< urls.size() << " sources for tesing "<< std::endl;

}
DeepstreamPipeline::~DeepstreamPipeline(){
 stop();
}

void DeepstreamPipeline::decodebin_child_added(GstChildProxy *child_proxy,GObject *object ,gchar *name,gpointer user_data){
    std::cout << "Decodebin child added "<< std::endl;
    if(g_strrstr(name,"decodebin")==name){
        g_signal_connect(G_OBJECT(object),"child-added",G_CALLBACK(decodebin_child_added),user_data);
    }
}
GstElement* DeepstreamPipeline::create_source_bin(guint index,const std::string &uri){
    GstElement *bin =nullptr,*uri_decode_bin=nullptr;
    std::string bin_name="source_bin-"+std::to_string(index);
    bin=gst_bin_new(bin_name.c_str());
    uri_decode_bin=gst_element_factory_make("nvurisrcbin","uri-decode-bin");
    g_object_set(G_OBJECT(uri_decode_bin),"rtsp-reconnect-interval",10,
                "rtsp-reconnect-attempts",-1,"latency",100,nullptr);
    
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

void DeepstreamPipeline::build(){
    gst_init(nullptr,nullptr);
    loop_=g_main_loop_new(nullptr,FALSE);
    pipeline_=gst_pipeline_new("test-pipeline");
    streammux=gst_element_factory_make("nvstreammux","stream-muxer");
    fakesink=gst_element_factory_make("fakesink","fakesink");
    if(!pipeline_ ||!streammux||!fakesink){
        std::cerr << "Build Error: failed to create element" << std::endl;
        return;
    }
    int batch_size=urls.size();
    g_object_set(streammux,"batch-size",batch_size,"width",1280,"height",720,"batched-push-timeout",40000,
                "enable-padding",TRUE,nullptr);
    g_object_set(fakesink,"sync",FALSE,"qos",TRUE,nullptr);

    gst_bin_add_many(GST_BIN(pipeline_),streammux,fakesink,nullptr);

    if(!gst_element_link(streammux,fakesink)){
        std::cerr << "Build Error: Failed to link muxer to sink" << std::endl;
        return;
    }
    for(int i=0;i< batch_size;i++){
         
        GstElement *source_bin=create_source_bin(i,urls[i]);
        if(!source_bin){
            std::cerr << "Build : Failed to create source bin "<< std::endl;
            return;
        }
        gst_bin_add(GST_BIN(pipeline_),source_bin);
        source_bins_.push_back(source_bin);
        std::string pad_name="sink_"+std::to_string(i);
        GstPad *sinkpad=gst_element_request_pad_simple(streammux,pad_name.c_str());
        GstPad *srcpad=gst_element_get_static_pad(source_bin,"src");
        if(!sinkpad || !srcpad){
            std::cerr << "Build: Failed to get pads for source "<< i << std::endl;
            return;
        }
        if(gst_pad_link(srcpad,sinkpad)!=GST_PAD_LINK_OK){
            std::cerr << "Build Failed to link pads source bin "<< i << std::endl;
            gst_object_unref(srcpad);
            gst_object_unref(sinkpad);
            return;
        }
        // g_object_set_data(G_OBJECT(source_bin),"sink-index",GINT_TO_POINTER(i));
        // g_signal_connect(source_bin,"pad-added",G_CALLBACK(on_pad_added),streammux);
    }
    bus_=gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    bus_watch_id=gst_bus_add_watch(bus_,bus_call,loop_);
    gst_object_unref(bus_);
    std::cout << "Build Test Pipeline build succefully "<< std::endl;  
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
    source_bins_.clear();
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
