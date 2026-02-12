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
void generate_ts_rfc3339(char *buf, int buf_size)
{
    time_t tloc;
    struct tm tm_log;
    struct timespec ts;
    char strmsec[6];

    clock_gettime(CLOCK_REALTIME, &ts);
    memcpy(&tloc, (void *)(&ts.tv_sec), sizeof(time_t));
    gmtime_r(&tloc, &tm_log);
    strftime(buf, buf_size, "%Y-%m-%dT%H:%M:%S", &tm_log);
    int ms = ts.tv_nsec / 1000000;
    g_snprintf(strmsec, sizeof(strmsec), ".%.3dZ", ms);
    strncat(buf, strmsec, buf_size);
}
std::string get_absolute_file_path(const std::string &cfg_file_path, const std::string &file_path)
{
    char abs_cfg_path[PATH_MAX + 1];

    if (!file_path.empty() && file_path[0] == '/')
    {
        return file_path;
    }

    if (!realpath(cfg_file_path.c_str(), abs_cfg_path))
    {
        return "";
    }

    // Return absolute path of config file if file_path is NULL.
    if (file_path.empty())
    {
        return abs_cfg_path;
    }
    std::string dir_path(abs_cfg_path);
    size_t last_slash = dir_path.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        dir_path = dir_path.substr(0, last_slash + 1);
    }
    return dir_path + file_path;
}
 
/* Meta data release function set by user */
void meta_free_func(gpointer data, gpointer user_data)
{
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *)user_meta->user_meta_data;

    if (srcMeta->ts)
    {
        g_free(srcMeta->ts);
    }

    if (srcMeta->sensorStr)
    {
        g_free(srcMeta->sensorStr);
    }

    if (srcMeta->objectId)
    {
        g_free(srcMeta->objectId);
    }

    g_free(srcMeta);
}
/* Meta data copy function set by user */
 gpointer meta_copy_func(gpointer data, gpointer user_data)
{
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *)user_meta->user_meta_data;
    NvDsEventMsgMeta *dstMeta = NULL;

    // Use g_memdup2 instead of deprecated g_memdup
    dstMeta = (NvDsEventMsgMeta *)g_memdup2(srcMeta, sizeof(NvDsEventMsgMeta));

    if (srcMeta->ts)
        dstMeta->ts = g_strdup(srcMeta->ts);

    if (srcMeta->sensorStr)
        dstMeta->sensorStr = g_strdup(srcMeta->sensorStr);

    if (srcMeta->objectId)
        dstMeta->objectId = g_strdup(srcMeta->objectId);

    return dstMeta;
}
 