#ifndef ANALYTICSCONFIGWRITER_H
#define ANALYTICSCONFIGWRITER_H
#include <glib.h>
#include <json-glib/json-glib.h>
#include <string>
#include <vector>

class AnalyticsConfigWriter{
    public:
        AnalyticsConfigWriter();
        ~AnalyticsConfigWriter();
        void setResolution(int width,int height);
        void setOsdMode(int mode);
        bool generateFromFile(const std::string &jsonInputPath,const std::string &outputConfigPath);
        bool generateFromString(const std::string &jsonconfig,const std::string &outputConfigPath);
    private:
        int m_configWidth;
        int m_configHeight;
        int m_osdMode;

        bool writeConfigFile(const std::string &filename ,GString *content);
        void printJsonObject(JsonObject *obj);
        void processRoiFiltering(JsonObject *config,GString *output,int streamIndex);
        void processLineCrossing(JsonObject *config,GString *output,int streamIndex);
        void processOvercrowding(JsonObject *config,GString *output,int streamIndex);
        void processDirectionDetection(JsonObject *config,GString *output,int streamIndex);
        
};

#endif 