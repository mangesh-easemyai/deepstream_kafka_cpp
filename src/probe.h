#ifndef PROBE_H
#define PROBE_H
#include <gst/gst.h>
#include <glib.h>
#include "nvbufsurface.h"
#include "gstnvdsmeta.h"
#include <nvds_obj_encode.h>
#include <gstnvdsmeta.h> 
#include "nvdsmeta.h"
#include "nvds_analytics_meta.h"
#include "nvdsmeta_schema.h"

#include <climits> // Add this for PATH_MAX
#include <json-glib/json-glib.h>
#include "../utils/common_utils.h"
#include "../utils/analyticsconfigwriter.h"
#ifndef MAX_TIME_STAMP_LEN
#define MAX_TIME_STAMP_LEN 32
#endif
GstPadProbeReturn nvdsanalytics_src_pad_buffer_probe(GstPad *pad,GstPadProbeInfo *info,gpointer u_data);
struct ProbeData
{
    NvDsObjEncCtxHandle obj_ctx_handle;
    JsonObject *root_obj;
     std::vector<std::string> source_ids; 
};

#endif