#include "pipeline.h"

DeepstreamPipeline::DeepstreamPipeline(std::vector<std::string> urls,guint rtsp_port,guint udp_port):urls_(urls),rtsp_port_(rtsp_port),udp_port_(udp_port){
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
    encoder_=gst_element_factory_make("nvv4l2h264enc","h264-encoder");
    parse_=gst_element_factory_make("h264parse","h264-parse");
    payloader_=gst_element_factory_make("rtph264pay","rtp-payer");
    udpsink_=gst_element_factory_make("udpsink","udp-sink");

    queue_encoder_=gst_element_factory_make("queue","queue_encoder");
    queue_parse_=gst_element_factory_make("queue","parse_encoder");
    queue_payloader_=gst_element_factory_make("queue","queue_payloader");

    if(!pipeline_ ||!streammux||!queue_encoder_||!encoder_ ||!queue_parse_ ||!parse_||!queue_payloader_ ||!payloader_||!udpsink_){
        std::cerr << "Build Error: failed to create element" << std::endl;
        return;
    }

    int batch_size=urls_.size();
    g_object_set(streammux,"batch-size",batch_size,"width",1280,"height",720,"batched-push-timeout",40000,
                "enable-padding",TRUE,nullptr);
    g_object_set(encoder_,"bitrate",4000000,
                "profile",0,nullptr);
    g_object_set(encoder_,"insert-sps-pps",1,"iframeinterval",30,"idrinterval",30,nullptr);
    g_object_set(payloader_,"config-interval",1,"pt",96,nullptr);

    g_object_set(queue_encoder_,"max-size-buffers",5,nullptr);
    g_object_set(queue_parse_,"max-size-buffers",5,nullptr);
    g_object_set(queue_payloader_,"max-size-buffers",5,nullptr);
    g_object_set(udpsink_,"host","127.0.0.1",
                "port",udp_port_,
                "sync",FALSE,
                "async",FALSE,nullptr);
    
        
    gst_bin_add_many(GST_BIN(pipeline_),streammux,queue_encoder_,encoder_,queue_parse_,parse_,queue_payloader_,payloader_,udpsink_,nullptr);

    if(!gst_element_link_many(streammux,queue_encoder_,encoder_,queue_parse_,parse_,queue_payloader_,payloader_,udpsink_,nullptr)){
        std::cerr << "Build Error: Failed to link muxer to sink" << std::endl;
        return;
    }
    for(int i=0;i< batch_size;i++){
         
        GstElement *source_bin=create_source_bin(i,urls_[i]);
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
         
    }
    bus_=gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    bus_watch_id=gst_bus_add_watch(bus_,bus_call,loop_);
    gst_object_unref(bus_);
    if(!setup_rtsp_server()){
        std::cerr << "Failed to start RTSP server "<< std::endl;
        return;
    }
    std::cout << "Build Test Pipeline build succefully "<< std::endl;  
}

gboolean DeepstreamPipeline::setup_rtsp_server(){
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    std::string udpsrc_pipeline;
    std::string port_num_str;
    guint64 udp_buffer_size = 512 * 1024;
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