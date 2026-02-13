#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include "pipeline/pipeline.h"
using json=nlohmann::json;


std::vector<std::string> get_url_from_json(){
    std::vector<std::string> urls;
    std::ifstream f("configs/app_config.json");
    if(!f.is_open()){
        std::cerr << "Failed to open app config.json"<< std::endl;
        return urls;
    }
    try{
        json data=json::parse(f);
        if(data.contains("data")&& data["data"].is_object()){
            for(auto &[camera_id,config]:data["data"].items()){
                if(config.contains("link")){
                    std::string link=config["link"];
                    urls.push_back(link);
                    std::cout << "Loaded URL for "<< camera_id << " : "<< link << std::endl;
                }
            }
        }
    }catch(json::parse_error& e){
        std::cerr << "Json Parse Error "<< e.what() << std::endl;
    }
    return urls;
}
int main(int argc,char** argv){
    gst_init(&argc,&argv);
    std::vector<std::string> uris=get_url_from_json();
    if(uris.empty()){
        std::cerr << "NO Urls founding in config. Exiting "<< std::endl;
        return -1;
    }
    guint rtsp_port=8554;
    guint udp_port=5400;
    DeepstreamPipeline pipeline(uris,rtsp_port,udp_port);
    pipeline.build();
    pipeline.run();

    return 0;
}