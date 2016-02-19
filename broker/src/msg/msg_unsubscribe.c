#include "broker/data/data.h"
#include "broker/stream.h"
#include "broker/net/ws.h"
#include "broker/msg/msg_unsubscribe.h"

static
void handle_unsubscribe(RemoteDSLink *link, uint32_t sid) {
    ref_t *ref = dslink_map_remove_get(&link->sub_sids, &sid);
    if (!ref) {
        return;
    }

    BrokerSubStream *bss = ref->data;
    dslink_map_remove(&bss->clients, &sid);
    if (bss->clients.size == 0) {
        dslink_map_remove(&bss->responder->sub_paths, bss->remote_path->data);
        dslink_map_remove(&bss->responder->sub_sids, &bss->responder_sid);

        json_t *top = json_object();
        json_t *reqs = json_array();
        json_object_set_new_nocheck(top, "requests", reqs);

        json_t *req = json_object();
        json_array_append_new(reqs, req);
        json_object_set_new_nocheck(req, "method",
                                    json_string_nocheck("unsubscribe"));
        {
            json_t *sids = json_array();
            json_array_append_new(sids, json_integer(bss->responder_sid));
            json_object_set_new_nocheck(req, "sids", sids);
        }

        broker_ws_send_obj(bss->responder, top);
        json_decref(top);

        broker_stream_free((BrokerStream *) bss);
    }

    dslink_decref(ref);
}

int broker_msg_handle_unsubscribe(RemoteDSLink *link, json_t *req) {
    broker_data_send_closed_resp(link, req);

    json_t *sids = json_object_get(req, "sids");
    if (sids) {
        size_t index;
        json_t *value;
        json_array_foreach(sids, index, value) {
            uint32_t sid = (uint32_t) json_integer_value(value);
            handle_unsubscribe(link, sid);
        }
    }

    return 0;
}
