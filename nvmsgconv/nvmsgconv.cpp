/*
 * SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */
// #include "nvds_analytics_meta.h"
#include "nvmsgconv.h"
#include "deepstream_schema.h"
#include <json-glib/json-glib.h>

#include <uuid.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <unordered_map>
#include "schema.pb.h"

using namespace std;


NvDsMsg2pCtx*
nvds_msg2p_ctx_create(const gchar* file, NvDsPayloadType type)
{
    NvDsMsg2pCtx* ctx = NULL;
    string str;
    bool retVal = true;
    std::cout << "nvds_msg2p_ctx_create\n";
    /*
     * Need to parse configuration / CSV files to get static properties of
     * components (e.g. sensor, place etc.) in case of full deepstream schema.
     */
    if (type == NVDS_PAYLOAD_DEEPSTREAM) {
        g_return_val_if_fail(file, NULL);

        ctx = new NvDsMsg2pCtx;
        ctx->privData = create_deepstream_schema_ctx();

        if (g_str_has_suffix(file, ".csv")) {
            retVal = nvds_msg2p_parse_csv(ctx->privData, file);
        } else if (g_str_has_suffix(file, ".yml") || g_str_has_suffix(file, ".yaml")) {
            retVal = nvds_msg2p_parse_yaml(ctx->privData, file);
        } else {
            retVal = nvds_msg2p_parse_key_value(ctx->privData, file);
        }
    } else {
        ctx = new NvDsMsg2pCtx;
        /* If configuration file is provided for minimal schema,
         * parse it for static values.
         */
        if (file) {
            ctx->privData = create_deepstream_schema_ctx();
            if (g_str_has_suffix(file, ".yml") || g_str_has_suffix(file, ".yaml")) {
                retVal = nvds_msg2p_parse_yaml(ctx->privData, file);
            } else {
                retVal = nvds_msg2p_parse_key_value(ctx->privData, file);
            }
        } else {
            ctx->privData = nullptr;
            retVal = true;
        }
    }

    ctx->payloadType = type;

    if (!retVal) {
        cout << "Error in creating instance" << endl;

        if (ctx && ctx->privData)
            destroy_deepstream_schema_ctx(ctx->privData);

        if (ctx) {
            delete ctx;
            ctx = NULL;
        }
    }

    return ctx;
}

void
nvds_msg2p_ctx_destroy(NvDsMsg2pCtx* ctx)
{
    destroy_deepstream_schema_ctx(ctx->privData);
    ctx->privData = nullptr;
    delete ctx;
}

NvDsPayload**
nvds_msg2p_generate_multiple(NvDsMsg2pCtx* ctx, NvDsEvent* events, guint eventSize, guint* payloadCount)
{
    gchar* message = NULL;
    size_t len = 0;
    NvDsPayload** payloads = NULL;
    *payloadCount = 0;
    // Set how many payloads are being sent back to the plugin
    payloads = (NvDsPayload**)g_malloc0(sizeof(NvDsPayload*) * 1);
    std::cout << "nvds_msg2p_generate_multiple\n";
    if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM) {
        message = generate_event_message(ctx->privData, events->metadata);
        if (message) {
            payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            len = strlen(message);
            // Remove '\0' character at the end of string and just copy the content.
            payloads[*payloadCount]->payload = g_memdup(message, len);
            payloads[*payloadCount]->payloadSize = len;
            ++(*payloadCount);
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM_MINIMAL) {
        message = generate_event_message_minimal(ctx->privData, events, eventSize);
        if (message) {
            len = strlen(message);
            payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            // Remove '\0' character at the end of string and just copy the content.
            payloads[*payloadCount]->payload = g_memdup(message, len);
            payloads[*payloadCount]->payloadSize = len;
            ++(*payloadCount);
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM_PROTOBUF) {
        message = generate_event_message_protobuf(ctx->privData, events, eventSize, len);
        if (message) {
            payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            payloads[*payloadCount]->payload = g_memdup(message, len);
            payloads[*payloadCount]->payloadSize = len;
            ++(*payloadCount);
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_CUSTOM) {
        payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
        payloads[*payloadCount]->payload = (gpointer)g_strdup("CUSTOM Schema");
        payloads[*payloadCount]->payloadSize = strlen((char*)payloads[*payloadCount]->payload) + 1;
        ++(*payloadCount);
    } else {
        g_free(payloads);
        payloads = NULL;
    }

    return payloads;
}

NvDsPayload*
nvds_msg2p_generate(NvDsMsg2pCtx* ctx, NvDsEvent* events, guint size)
{
    gchar* message = NULL;
    size_t len = 0;
    NvDsPayload* payload = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
    std::cout << "nvds_msg2p_generate\n";
    if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM) {
        message = generate_event_message(ctx->privData, events->metadata);
        if (message) {
            len = strlen(message);
            // Remove '\0' character at the end of string and just copy the content.
            payload->payload = g_memdup(message, len);
            payload->payloadSize = len;
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM_MINIMAL) {
        message = generate_event_message_minimal(ctx->privData, events, size);
        if (message) {
            len = strlen(message);
            // Remove '\0' character at the end of string and just copy the content.
            payload->payload = g_memdup(message, len);
            payload->payloadSize = len;
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM_PROTOBUF) {
        message = generate_event_message_protobuf(ctx->privData, events, size, len);
        if (message) {
            payload->payload = g_memdup(message, len);
            payload->payloadSize = len;
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_CUSTOM) {
        payload->payload = (gpointer)g_strdup("CUSTOM Schema");
        payload->payloadSize = strlen((char*)payload->payload) + 1;
    } else
        payload->payload = NULL;

    return payload;
}

NvDsPayload**
nvds_msg2p_generate_multiple(NvDsMsg2pCtx* ctx, NvDsEvent* events, guint eventSize, guint* payloadCount)
{
    *payloadCount = 0;
    
    if (!events || eventSize == 0) {
        return NULL;
    }

    // Allocate space for 1 payload (we return 1 message per frame)
    NvDsPayload **payloads = (NvDsPayload**)g_malloc0(sizeof(NvDsPayload*) * 1);

    // Find first valid CUSTOM event with extMsg
    for (guint i = 0; i < eventSize; i++) {
        NvDsEventMsgMeta *meta = events[i].metadata;
        
        if (!meta) continue;

        if (meta->type == NVDS_EVENT_CUSTOM && meta->extMsg && meta->extMsgSize > 0) {
            NvDsPayload *payload = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            
            size_t len = meta->extMsgSize > 0 ? meta->extMsgSize - 1 : 0;
            
            payload->payload = g_memdup(meta->extMsg, len);
            payload->payloadSize = len;
            
            payloads[0] = payload;
            *payloadCount = 1;
            
            return payloads;  // Return immediately after finding first valid event
        }
    }

    // No valid custom event found
    g_free(payloads);
    return NULL;
}

NvDsPayload**
nvds_msg2p_generate_multiple_new(NvDsMsg2pCtx* ctx, void* metadataInfo, guint* payloadCount)
{
    gchar* message = NULL;
    size_t len = 0;
    NvDsPayload** payloads = NULL;
    *payloadCount = 0;
    // Set how many payloads are being sent back to the plugin
    payloads = (NvDsPayload**)g_malloc0(sizeof(NvDsPayload*) * 1);

    NvDsMsg2pMetaInfo* meta_info = (NvDsMsg2pMetaInfo*)metadataInfo;
    NvDsFrameMeta* frame_meta = (NvDsFrameMeta*)meta_info->frameMeta;
    NvDsObjectMeta* obj_meta = (NvDsObjectMeta*)meta_info->objMeta;
    std::cout << "nvds_msg2p_generate_multiple_new\n";
    if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM) {
        message = generate_dsmeta_message(ctx->privData, frame_meta, obj_meta);
        if (message) {
            payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            len = strlen(message);
            // Remove '\0' character at the end of string and just copy the content.
            payloads[*payloadCount]->payload = g_memdup(message, len);
            payloads[*payloadCount]->payloadSize = len;
            ++(*payloadCount);
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM_MINIMAL) {
        if (meta_info->datamap) {
            message = generate_dsmeta_message_ds3d(ctx->privData, meta_info->datamap, FALSE, len);
        } else {
            message = generate_dsmeta_message_minimal(ctx->privData, frame_meta);
        }
        if (message) {
            len = strlen(message);
            payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            // Remove '\0' character at the end of string and just copy the content.
            payloads[*payloadCount]->payload = g_memdup(message, len);
            payloads[*payloadCount]->payloadSize = len;
            ++(*payloadCount);
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_DEEPSTREAM_PROTOBUF) {
        if (meta_info->datamap) {
            message = generate_dsmeta_message_ds3d(ctx->privData, meta_info->datamap, TRUE, len);
        } else {
            message = generate_dsmeta_message_protobuf(ctx->privData, frame_meta, len);
        }
        if (message) {
            payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
            payloads[*payloadCount]->payload = g_memdup(message, len);
            payloads[*payloadCount]->payloadSize = len;
            ++(*payloadCount);
            g_free(message);
        }
    } else if (ctx->payloadType == NVDS_PAYLOAD_CUSTOM) {
        payloads[*payloadCount] = (NvDsPayload*)g_malloc0(sizeof(NvDsPayload));
        payloads[*payloadCount]->payload = (gpointer)g_strdup("CUSTOM Schema");
        payloads[*payloadCount]->payloadSize = strlen((char*)payloads[*payloadCount]->payload) + 1;
        ++(*payloadCount);
    } else {
	g_free(payloads);
        payloads = NULL;
    }

    return payloads;
}

void
nvds_msg2p_release(NvDsMsg2pCtx* ctx, NvDsPayload* payload)
{
    g_free(payload->payload);
    g_free(payload);
}
