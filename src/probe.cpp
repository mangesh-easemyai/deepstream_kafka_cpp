#include "probe.h"

#include <iostream>
gint frame_number = 0 ;
std::vector<std::string> class_labels=read_class_label("configs/Primary_Detector/labels.txt");
GstPadProbeReturn nvdsanalytics_src_pad_buffer_probe(GstPad *pad,GstPadProbeInfo *info,gpointer u_data){
    GstBuffer *buf=(GstBuffer *)info->data;
    GstMapInfo inmap=GST_MAP_INFO_INIT;
    ProbeData *probe_data=static_cast<ProbeData *>(u_data);
    if(!probe_data){
        return GST_PAD_PROBE_OK;
    }
    NvDsObjEncCtxHandle enc_ctx=probe_data->obj_ctx_handle;
    JsonObject *root_obj=probe_data->root_obj;
    if(!gst_buffer_map(buf,&inmap,GST_MAP_READ)){
        GST_ERROR("Failed to map GstBuffer");
        return GST_PAD_PROBE_OK;
    }
    NvBufSurface *ip_surf=(NvBufSurface *)inmap.data;
    gst_buffer_unmap(buf,&inmap);
    NvDsBatchMeta *batch_meta=gst_buffer_get_nvds_batch_meta(buf);
    if(!batch_meta){
        return GST_PAD_PROBE_OK;
    }
    const gchar *calc_enc_str=g_getenv("CALCULATE_ENCODE_TIME");
    gboolean cal_enc=!g_strcmp0(calc_enc_str,"yes");
    if(!root_obj || !json_object_has_member(root_obj,"service_id")||!json_object_has_member(root_obj,"data")){
        return GST_PAD_PROBE_OK;
    }
    const char *service_id=json_object_get_string_member(root_obj,"service_id");
    JsonObject *data_obj=json_object_get_object_member(root_obj,"data");
    GList *ids=json_object_get_members(data_obj);
    for(NvDsMetaList *l_frame=batch_meta->frame_meta_list;l_frame !=NULL;l_frame=l_frame->next){
        NvDsFrameMeta *frame_meta=(NvDsFrameMeta *)l_frame->data;
        int source_id=frame_meta->source_id;
        std::string id_str;
        if(source_id >=0 && source_id <probe_data->source_ids.size()){
            id_str=probe_data->source_ids[source_id];
        }else{
            id_str="unknown_"+std::to_string(source_id);
        }

        const char *id=id_str.c_str();
        JsonObject *entry=nullptr;
        if(json_object_has_member(data_obj,id)){
            entry=json_object_get_object_member(data_obj,id);
        }
        const char *link=nullptr;
        if(json_object_has_member(entry,"link")){
            link=json_object_get_string_member(entry,"link");
        }
        NvDsObjEncUsrArgs frameData={0};
        frameData.isFrame=1;
        frameData.saveImg=FALSE;
        frameData.attachUsrMeta=TRUE;
        frameData.scaleImg=FALSE;
        frameData.quality=50;

        if(cal_enc){
            frameData.calcEncodeTime =1;
        }
        nvds_obj_enc_process(enc_ctx,&frameData,ip_surf,NULL,frame_meta);
        nvds_obj_enc_finish(enc_ctx);
        gchar ts[MAX_TIME_STAMP_LEN +1];
        generate_ts_rfc3339_(ts,MAX_TIME_STAMP_LEN);

        JsonObject *rootObj=json_object_new();
        json_object_set_string_member(rootObj,"timestamp",ts);
        json_object_set_string_member(rootObj,"source_id",id);
        json_object_set_string_member(rootObj,"service_id",service_id);
        json_object_set_string_member(rootObj,"source_name",link);

        JsonObject *taskDetailsObj=json_object_new();
        JsonArray *objectsArray =json_array_new();

        float scaleW=(float)frame_meta->source_frame_width /((frame_meta->pipeline_width ==0)? 1:frame_meta->pipeline_width);
        float scaleH=(float)frame_meta->source_frame_height/((frame_meta->pipeline_height==0)? 1:frame_meta->pipeline_height);

        for(NvDsMetaList *l_obj=frame_meta->obj_meta_list;l_obj !=NULL;l_obj=l_obj->next){
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)l_obj->data;
            JsonObject *objJson = json_object_new();
            json_object_set_int_member(objJson, "object_id", obj_meta->object_id);
            json_object_set_int_member(objJson, "objClassId", obj_meta->class_id);
            const char* class_name="Unknown";
            if(obj_meta->class_id < class_labels.size()){
                class_name=class_labels[obj_meta->class_id].c_str();
            }
            json_object_set_string_member(objJson, "objclass", class_name);
            json_object_set_double_member(objJson, "confidence", obj_meta->confidence);

            JsonObject *bboxObj = json_object_new();
            json_object_set_double_member(bboxObj, "top", obj_meta->rect_params.top * scaleH);
            json_object_set_double_member(bboxObj, "left", obj_meta->rect_params.left * scaleW);
            json_object_set_double_member(bboxObj, "width", obj_meta->rect_params.width * scaleW);
            json_object_set_double_member(bboxObj, "height", obj_meta->rect_params.height * scaleH);

            json_object_set_object_member(objJson, "bbox", bboxObj);
            json_array_add_object_element(objectsArray, objJson);
        }
        // Process analytics metadata (existing code continues...)
        JsonObject *analyticKeyFrameObj = json_object_new();
        for (NvDsMetaList *l_user = frame_meta->frame_user_meta_list; l_user != NULL; l_user = l_user->next)
        {
            NvDsUserMeta *user_meta = (NvDsUserMeta *)l_user->data;
            if (user_meta->base_meta.meta_type == nvds_get_user_meta_type((gchar *)"NVIDIA.DSANALYTICSFRAME.USER_META"))
            {
                NvDsAnalyticsFrameMeta *analytics_meta = (NvDsAnalyticsFrameMeta *)user_meta->user_meta_data;

                // ROI counts (existing code)
                if (analytics_meta->objInROIcnt.empty())
                {
                    json_object_set_null_member(analyticKeyFrameObj, "objs_in_ROI");
                }
                else
                {
                    JsonObject *roiObj = json_object_new();
                    for (const auto &kv : analytics_meta->objInROIcnt)
                    {
                        json_object_set_int_member(roiObj, kv.first.c_str(), kv.second);
                    }
                    json_object_set_object_member(analyticKeyFrameObj, "objs_in_ROI", roiObj);
                }

                // Line crossing cumulative
                if (analytics_meta->objLCCumCnt.empty())
                {
                    json_object_set_null_member(analyticKeyFrameObj, "linecrossing_cumulative");
                }
                else
                {
                    JsonObject *lcCumobj = json_object_new();
                    for (const auto &kv : analytics_meta->objLCCumCnt)
                    {
                        json_object_set_int_member(lcCumobj, kv.first.c_str(), kv.second);
                    }
                    json_object_set_object_member(analyticKeyFrameObj, "linecrossing_cumulative", lcCumobj);
                }

                // Line crossing current frame
                if (analytics_meta->objLCCurrCnt.empty())
                {
                    json_object_set_null_member(analyticKeyFrameObj, "linecrossing_current_frame");
                }
                else
                {
                    JsonObject *lcCurrObj = json_object_new();
                    for (const auto &kv : analytics_meta->objLCCurrCnt)
                    {
                        json_object_set_int_member(lcCurrObj, kv.first.c_str(), kv.second);
                    }
                    json_object_set_object_member(analyticKeyFrameObj, "linecrossing_current_frame", lcCurrObj);
                }

                // Overcrowding
                if (analytics_meta->ocStatus.empty())
                {
                    json_object_set_null_member(analyticKeyFrameObj, "overcrowding_status");
                }
                else
                {
                    JsonObject *ocObj = json_object_new();
                    for (const auto &kv : analytics_meta->ocStatus)
                    {
                        json_object_set_int_member(ocObj, kv.first.c_str(), kv.second);
                    }
                    json_object_set_object_member(analyticKeyFrameObj, "overcrowding_status", ocObj);
                }
                break;
            }
        }

        if (json_object_get_size(analyticKeyFrameObj) == 0)
        {
            json_object_set_null_member(analyticKeyFrameObj, "objs_in_ROI");
            json_object_set_null_member(analyticKeyFrameObj, "linecrossing_cumulative");
            json_object_set_null_member(analyticKeyFrameObj, "linecrossing_current_frame");
            json_object_set_null_member(analyticKeyFrameObj, "overcrowding_status");
        }

        gchar frameKey[32];
        g_snprintf(frameKey, sizeof(frameKey), "%u", frame_meta->frame_num);
        json_object_set_string_member(taskDetailsObj, "frame_id", frameKey);
        json_object_set_array_member(taskDetailsObj, "detections", objectsArray);
        json_object_set_object_member(taskDetailsObj, "analytic_key_frame", analyticKeyFrameObj);
        json_object_set_object_member(rootObj, "task_details", taskDetailsObj);
        


        JsonNode *rootNode = json_node_new(JSON_NODE_OBJECT);
        json_node_set_object(rootNode, rootObj);
        gchar *frame_json_str = json_to_string(rootNode, TRUE);
         // Create NvDsEventMsgMeta with JSON in extMsg
        NvDsEventMsgMeta *msg_meta = (NvDsEventMsgMeta *)g_malloc0(sizeof(NvDsEventMsgMeta));
        msg_meta->type = NVDS_EVENT_CUSTOM;
        msg_meta->objType = NVDS_OBJECT_TYPE_CUSTOM;
        msg_meta->frameId = frame_meta->frame_num;
        msg_meta->ts = g_strdup(ts);
        msg_meta->sensorId = source_id;
        msg_meta->sensorStr = g_strdup("camera_id");
        msg_meta->videoPath = g_strdup("source_name");
        msg_meta->extMsg = (void *)g_strdup(frame_json_str);
        msg_meta->extMsgSize = strlen(frame_json_str) + 1;

        // Cleanup
         
        g_free(frame_json_str);
        json_node_free(rootNode);
        // json_object_unref(rootObj);
        NvDsUserMeta *user_event_meta = nvds_acquire_user_meta_from_pool(batch_meta);
        if (user_event_meta)
        {
            user_event_meta->user_meta_data = (void *)msg_meta;
            user_event_meta->base_meta.meta_type = NVDS_EVENT_MSG_META;
            user_event_meta->base_meta.copy_func = (NvDsMetaCopyFunc)meta_copy_func;
            user_event_meta->base_meta.release_func = (NvDsMetaReleaseFunc)meta_free_func;
            nvds_add_user_meta_to_frame(frame_meta, user_event_meta);
        }
        else
        {
            if (msg_meta->ts)
                g_free(msg_meta->ts);
            if (msg_meta->sensorStr)
                g_free(msg_meta->sensorStr);
            if (msg_meta->videoPath)
                g_free(msg_meta->videoPath);
            if (msg_meta->extMsg)
                g_free(msg_meta->extMsg);
            g_free(msg_meta);
        }
    }
    
    frame_number++;
    g_list_free(ids);
    return GST_PAD_PROBE_OK;
}