#ifndef PIPELINE_H
#define PIPELINE_H
#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <vector>
#include <string>

#include "nvbufsurface.h"
#include "gstnvdsmeta.h"
 

class DeepstreamPipeline{
    public:
        DeepstreamPipeline(std::vector<std::string> urls);
        ~DeepstreamPipeline();
        void build();
        void run();
        void stop();
    private:
        GMainLoop *loop_=nullptr;
        GstElement *pipeline_=nullptr,*streammux=nullptr,*fakesink=nullptr;
        GstBus *bus_=nullptr;

        std::vector<std::string> urls;
        std::vector<GstElement*> source_bins_;

        guint gpu_id=0;
        guint bus_watch_id;
        
        GstElement* create_source_bin(guint index,const std::string &uri);

        static void cb_newpad(GstElement *decodebin,GstPad *decoder_src_pad,gpointer data);
        static void decodebin_child_added(GstChildProxy *child_proxy,GObject *object ,gchar *name,gpointer user_data);
        static void on_pad_added(GstElement *src,GstPad *pad,gpointer data);
        static gboolean bus_call(GstBus *bus ,GstMessage *msg,gpointer data);        
};

#endif