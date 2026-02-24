#include <iostream>
#include <filesystem>
#include <fstream>
#include <sys/stat.h> // for mkdir
#include <sstream>
#include <fstream>
#include <vector>
#include <time.h>
#include <cstring>
#include <sys/time.h>
#include <json-glib/json-glib.h>
#include "gstnvdsmeta.h"
#include "nvdsmeta_schema.h"
 

namespace fs = std::filesystem;
bool ensure_directory(const std::string &dir)
{
    try
    {
        if (!fs::exists(dir))
        {
            if (fs::create_directories(dir))
            {
                std::cout << "Created directory: " << dir << std::endl;
            }
            else
            {
                std::cerr << "Failed to created directory: " << dir << std::endl;
                return false;
            }
        }
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Fileystem Error : " << e.what() << std::endl;
        return false;
    }
}
std::vector<std::string> read_class_label(const std::string &filename)
{
    std::vector<std::string> labels;
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line))
    {
        if (!line.empty())
        {
            labels.push_back(line);
        }
    }
    return labels;
}

/* Function to generate RFC3339 timestamp */
  void generate_ts_rfc3339_(char *ts, size_t size)
{
     GDateTime *dt=g_date_time_new_now_local();
     if(dt){
        gchar *tmp=g_date_time_format(dt,"%Y-%m-%dT%H:%M:%S%z");
        if(tmp){
            g_strlcpy(ts,tmp,size);
            g_free(tmp);
        }
        g_date_time_unref(dt);
     }
}
std::string get_absolute_file_path(const std::string &cfg_file_path, const std::string &file_path)
{
     if(file_path.empty()){
        return " ";
     }
     //if path is already absolute ,return it
     if(file_path[0]=='/'){
        return file_path;
     }
     std::string dir=cfg_file_path.substr(0,cfg_file_path.find_last_of("/\\"));
     if(dir.empty()){
        dir=".";
     }
     std::string abs_path=dir+"/"+file_path;
     //simple check to see if the constructed file exist
     struct stat buffer;
     if(stat(abs_path.c_str(),&buffer)==0){
        return abs_path;
     }
     //if not found ,return the original path 
     return file_path;
}
 
/* Meta data release function set by user */
void meta_free_func(gpointer data, gpointer user_data)
{
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsEventMsgMeta *srcMeta=(NvDsEventMsgMeta *)user_meta->user_meta_data;

    // Use g_memdup2 instead of deprecated g_memdup
    // dstMeta = (NvDsEventMsgMeta *)g_memdup2(srcMeta, sizeof(NvDsEventMsgMeta));

    if (srcMeta->ts){
        // dstMeta->ts = g_strdup(srcMeta->ts);
        g_free(srcMeta->ts);
    }
    if (srcMeta->sensorStr){
        // dstMeta->sensorStr = g_strdup(srcMeta->sensorStr);
        g_free(srcMeta->sensorStr);
    }
    if (srcMeta->objectId){
        // dstMeta->objectId = g_strdup(srcMeta->objectId);
        g_free(srcMeta->objectId);
    }
    if(srcMeta->videoPath){
        g_free(srcMeta->videoPath);
    }
    if(srcMeta->extMsg){
        g_free(srcMeta->extMsg);
    }

    g_free(srcMeta);
}
/* Meta data copy function set by user */
 gpointer meta_copy_func(gpointer data, gpointer user_data)
{   
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *)user_meta->user_meta_data;
    NvDsEventMsgMeta *dstMeta = NULL;

    // FIX: Use g_memdup for GLib < 2.68 compatibility (common on DeepStream platforms)
    dstMeta = (NvDsEventMsgMeta *)g_memdup2(srcMeta, sizeof(NvDsEventMsgMeta));

    if (srcMeta->ts)
        dstMeta->ts = g_strdup(srcMeta->ts);

    if (srcMeta->sensorStr)
        dstMeta->sensorStr = g_strdup(srcMeta->sensorStr);

    if (srcMeta->objectId)
        dstMeta->objectId = g_strdup(srcMeta->objectId);
    
    if (srcMeta->videoPath)
        dstMeta->videoPath = g_strdup(srcMeta->videoPath);
    
    if (srcMeta->extMsg)
        dstMeta->extMsg = g_strdup((char*)srcMeta->extMsg);

    return dstMeta;
}