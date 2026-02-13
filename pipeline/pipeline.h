#ifndef PIPELINE_H
#define PIPELINE_H
#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <vector>
#include <string>

#include <gst/rtsp-server/rtsp-server.h>
#include "nvbufsurface.h"
#include "gstnvdsmeta.h"
 

class DeepstreamPipeline{
    public:
        DeepstreamPipeline(std::vector<std::string> urls_,guint rtsp_port_,guint udp_port_);
        ~DeepstreamPipeline();
        void build();
        void run();
        void stop();
    private:
        GMainLoop *loop_=nullptr;
        GstElement *pipeline_=nullptr,*streammux=nullptr,*fakesink=nullptr;
        GstElement *encoder_=nullptr,*parse_=nullptr,*payloader_=nullptr,*udpsink_=nullptr;
        GstElement *queue_encoder_=nullptr,*queue_payloader_=nullptr,*queue_parse_=nullptr;
        GstBus *bus_=nullptr;

        GstRTSPServer *server_=nullptr;
        std::vector<std::string> urls_;
        std::string service_id_="ds-test";
        guint rtsp_port_;
        guint udp_port_;
        std::vector<GstElement*> source_bins_;

        guint gpu_id=0;
        guint bus_watch_id;
        
        GstElement* create_source_bin(guint index,const std::string &uri);
        gboolean setup_rtsp_server();
        static void cb_newpad(GstElement *decodebin,GstPad *decoder_src_pad,gpointer data);
        static void decodebin_child_added(GstChildProxy *child_proxy,GObject *object ,gchar *name,gpointer user_data);
        static void on_pad_added(GstElement *src,GstPad *pad,gpointer data);
        static gboolean bus_call(GstBus *bus ,GstMessage *msg,gpointer data);        
};

#endif