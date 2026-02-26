#ifndef PIPELINE_H
#define PIPELINE_H
#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <vector>
#include <string>
#include <cmath>

#include <gst/rtsp-server/rtsp-server.h>
#include "nvbufsurface.h"
#include "gstnvdsmeta.h"

#include "../utils/common_utils.h"
#include "../src/probe.h"
#include "../utils/analyticsconfigwriter.h"
class DeepstreamPipeline{
    public:
        DeepstreamPipeline(JsonObject *config_root,guint rtsp_port_=8554,guint udp_port_=5400,std::string infer_config_path_="configs/Primary_Detector/config_infer_triton_yolov8.txt",std::string tracker_config_path_="configs/tracker_config/tracker_config.txt",std::string analytics_config_path_="configs/config_nvdsanalytics.txt");
        ~DeepstreamPipeline();
        void build();
        void run();
        void stop();
    private:
        GMainLoop *loop_=nullptr;
        GstElement *pipeline_=nullptr,*streammux=nullptr,*tiler_=nullptr,*primary_nvinference_=nullptr,*nvosd_=nullptr,*nvtracker_=nullptr,*nvdsanalytics_=nullptr;
        GstElement *tee_=nullptr,*queue_display_=nullptr,*queue_kafka_=nullptr,*nvmsgconv_=nullptr,*nvmsgbroker_=nullptr;
        GstElement *encoder_=nullptr,*parse_=nullptr,*payloader_=nullptr,*udpsink_=nullptr;
        GstElement *queue_encoder_=nullptr,*queue_payloader_=nullptr,*queue_parse_=nullptr,*queue_tiler_=nullptr,*queue_infer_=nullptr,*queue_osd_=nullptr,*queue_tracker=nullptr;
        JsonObject *config_root_;
        GstBus *bus_=nullptr;
        gint muxer_width_=1280;
        gint muxer_height_=720;
        GstRTSPServer *server_=nullptr;
        std::vector<std::string> urls_;
        std::string tracker_config_path_;
        std::string analytics_config_path_;
        std::string service_id_="ds-test";
        std::string infer_config_path_;
        guint rtsp_port_;
        guint udp_port_;
        std::vector<GstElement*> source_bins_;

        guint gpu_id=0;
        guint bus_watch_id;
        
        GstElement* create_source_bin(guint index,const std::string &uri);
        gboolean setup_rtsp_server();
        bool set_tracker_properties(GstElement *nvtracker);
        static void cb_newpad(GstElement *decodebin,GstPad *decoder_src_pad,gpointer data);
        static void decodebin_child_added(GstChildProxy *child_proxy,GObject *object ,gchar *name,gpointer user_data);
        static void on_pad_added(GstElement *src,GstPad *pad,gpointer data);
        static gboolean bus_call(GstBus *bus ,GstMessage *msg,gpointer data);        
};

#endif