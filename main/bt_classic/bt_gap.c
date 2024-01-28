#include "../main.h"
#include "bt_classic_private.h"

static const char *TAG = "BT_GAP";


static char bda_str[18];


void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {

    switch (event) {

        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_DISC_STATE_CHANGED_EVT", __func__);

            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
                ESP_LOGD(TAG, "  --discovery started");
            }
            else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                ESP_LOGD(TAG, "  --discovery stopped");
                hidlink_set_command(HIDLINK_COMMAND_SCAN_DONE);
            }
            else {
                ESP_LOGD(TAG, "  --unknown discovery state: %d", param->disc_st_chg.state);
            }
            break;
        }

        case ESP_BT_GAP_DISC_RES_EVT: {

            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_DISC_RES_EVT", __func__);

            uint32_t i;
            esp_bt_gap_dev_prop_t *property;
            uint32_t cod = 0;
            int32_t rssi = -129; /* invalid value */
            uint8_t *bdname = NULL;
            uint32_t bdname_len = 0;
            uint8_t *eir = NULL;
            uint8_t eir_len = 0;
            
            ESP_LOGI(TAG, "device found: %s", bda2str(param->disc_res.bda, bda_str, 18));
            ESP_LOGD(TAG, "  --number of properties: %d", param->disc_res.num_prop);
            
            for (i = 0; i < param->disc_res.num_prop; i++) {
                
                property = param->disc_res.prop + i;

                switch (property->type) {

                    case ESP_BT_GAP_DEV_PROP_COD: {
                        cod = *(uint32_t *)(property->val);
                        ESP_LOGD(TAG, "  --class of device: 0x%"PRIx32, cod);
                        break;
                    }

                    case ESP_BT_GAP_DEV_PROP_RSSI: {
                        rssi = *(int8_t *)(property->val);
                        ESP_LOGD(TAG, "  --rssi: %"PRId32, rssi);
                        break;
                    }

                    case ESP_BT_GAP_DEV_PROP_BDNAME: {
                        if (property->len < ESP_BT_GAP_MAX_BDNAME_LEN)
                            bdname_len = property->len;
                        else
                            bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
                        bdname = (uint8_t *)(property->val);
                        ESP_LOGD(TAG, "  --name: %s", bdname);
                        break;
                    }

                    case ESP_BT_GAP_DEV_PROP_EIR: {
                        eir_len = property->len;
                        eir = (uint8_t *)(property->val);
                        ESP_LOGD(TAG, "  --extended inquiry response: %d bytes", eir_len);
                        ESP_LOG_BUFFER_HEX_LEVEL(TAG, eir, eir_len, ESP_LOG_DEBUG);
                        break;
                    }

                    default: {
                        ESP_LOGD(TAG, "  --unexpected property: %d", property->type);
                        break;
                    }
                }
            }

            if (!esp_bt_gap_is_valid_cod(cod)) {

                ESP_LOGD(TAG, "  --invalid class of device: 0x%04lx", cod);
            }
            else if (esp_bt_gap_get_cod_major_dev(cod) != ESP_BT_COD_MAJOR_DEV_PERIPHERAL) {
                
                ESP_LOGD(TAG, "  --device is not PERIPHERAL");
            }
            else if (bdname_len == 0) {

                ESP_LOGD(TAG, "  --name not available, requesting remote device name");
                esp_bt_gap_read_remote_name(param->disc_res.bda);
            }
            else {
                hidlink_add_hid_peripheral(&param->disc_res.bda, (char *) bdname);
                esp_bt_gap_get_remote_services(param->disc_res.bda);
            }

            break;
        }

        case ESP_BT_GAP_READ_REMOTE_NAME_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_READ_REMOTE_NAME_EVT", __func__);
            ESP_LOGD(TAG, "  --name: %s", param->read_rmt_name.rmt_name);
            if (hidlink_get_status() == HIDLINK_STATUS_SCANNING) {
                hidlink_add_hid_peripheral(&param->read_rmt_name.bda, (char *) param->read_rmt_name.rmt_name);
                //esp_bt_gap_get_remote_services(param->disc_res.bda);
            }
            // else if (hidlink_get_status() == HIDLINK_STATUS_CONNECTED) {
            //     hidlink_set_attached_device_bda(&param->read_rmt_name.bda);
            //     hidlink_set_attached_device_name((char *) param->read_rmt_name.rmt_name);
            //     hidlink_set_command(HIDLINK_COMMAND_SHOW_ATTACHED_DEVICE_INFO);
            // }
            break;
        }

        case ESP_BT_GAP_RMT_SRVCS_EVT: {
            int i;
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_RMT_SRVCS_EVT", __func__);
            ESP_LOGD(TAG, "rmt_srvcs.bda: %s", bda2str(param->rmt_srvcs.bda, bda_str, 18));
            ESP_LOGD(TAG, "rmt_srvcs.stat: %lu", (uint32_t) param->rmt_srvcs.stat);
            ESP_LOGD(TAG, "rmt_srvcs.num_uuids: %lu", (uint32_t) param->rmt_srvcs.num_uuids);
            for(i = 0; i < param->rmt_srvcs.num_uuids; i++) {
                ESP_LOGD(TAG, "rmt_srvcs.uuid_list[%lu]:", (uint32_t) i);
                ESP_LOGD(TAG, "  - len: %u", (uint16_t) param->rmt_srvcs.uuid_list[i].len);
                ESP_LOGD(TAG, "  - uuid16: %04x", (uint16_t) param->rmt_srvcs.uuid_list[i].uuid.uuid16);
                ESP_LOGD(TAG, "  - uuid32: %08lx", (uint32_t) param->rmt_srvcs.uuid_list[i].uuid.uuid32);
            }
            break;
        }
    
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT", __func__);
            ESP_LOGD(TAG, "  - stat: %lu", (uint32_t) param->acl_conn_cmpl_stat.stat);
            ESP_LOGD(TAG, "  - handle: %lu", (uint32_t) param->acl_conn_cmpl_stat.handle);
            ESP_LOGD(TAG, "  - bda: %s", bda2str(param->acl_conn_cmpl_stat.bda, bda_str, 18));
            // if (hidlink_get_status() == HIDLINK_STATUS_TRYING_TO_CONNECT) {
            //     hidlink_set_command(HIDLINK_COMMAND_SET_STATUS_CONNECTED);
            //     esp_bt_gap_read_remote_name(param->acl_conn_cmpl_stat.bda);
            // }
            // else {
            //     ESP_LOGD(TAG, "trying to disconnect");
            //     esp_bt_hid_host_disconnect(param->acl_conn_cmpl_stat.bda);
            // }
            break;
        }

        case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT", __func__);
            ESP_LOGD(TAG, "  - reason: %lu", (uint32_t) param->acl_disconn_cmpl_stat.reason);
            ESP_LOGD(TAG, "  - handle: %lu", (uint32_t) param->acl_disconn_cmpl_stat.handle);
            ESP_LOGD(TAG, "  - bda: %s", bda2str(param->acl_disconn_cmpl_stat.bda, bda_str, 18));
            break;
        }

        case ESP_BT_GAP_MODE_CHG_EVT: {
            char *mode_str[4] = {
                "ESP_BT_PM_MD_ACTIVE",
                "ESP_BT_PM_MD_HOLD",
                "ESP_BT_PM_MD_SNIFF",
                "ESP_BT_PM_MD_PARK"
            };

            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_MODE_CHG_EVT", __func__);
            ESP_LOGD(TAG, "  - bda: %s", bda2str(param->mode_chg.bda, bda_str, 18));
            ESP_LOGD(TAG, "  - mode: %lu %s", (uint32_t) param->mode_chg.mode, mode_str[param->mode_chg.mode]);
            break;
        }
        
        case ESP_BT_GAP_RMT_SRVC_REC_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_RMT_SRVC_REC_EVT", __func__);
            break;
        }

        case ESP_BT_GAP_AUTH_CMPL_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_AUTH_CMPL_EVT", __func__);
            ESP_LOGD(TAG, "  - bda: %s", bda2str(param->auth_cmpl.bda, bda_str, 18));
            ESP_LOGD(TAG, "  - stat: %lu", (uint32_t) param->auth_cmpl.stat);
            ESP_LOGD(TAG, "  - name: %s", param->auth_cmpl.device_name);
            break;
        }

        case ESP_BT_GAP_CONFIG_EIR_DATA_EVT: {
            uint32_t i;
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_CONFIG_EIR_DATA_EVT", __func__);
            ESP_LOGD(TAG, "  - stat: %lu", (uint32_t) param->config_eir_data.stat);
            ESP_LOGD(TAG, "  - num: %lu", (uint32_t) param->config_eir_data.eir_type_num);
            for (i = 0; i < param->config_eir_data.eir_type_num; i++) {
                ESP_LOGD(TAG, "    - type %lu: %lu", (uint32_t) i, (uint32_t) param->config_eir_data.eir_type[i]);
            }
            break;
        }

        case ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT", __func__);
            ESP_LOGD(TAG, "  - bda: %s", bda2str(param->remove_bond_dev_cmpl.bda, bda_str, 18));
            ESP_LOGD(TAG, "  - status: %lu", (uint32_t) param->remove_bond_dev_cmpl.status);
            break;
        }

        case ESP_BT_GAP_KEY_NOTIF_EVT: {
            ESP_LOGD(TAG, "%s, event ESP_BT_GAP_KEY_NOTIF_EVT", __func__);
            ESP_LOGD(TAG, "  - bda: %s", bda2str(param->key_notif.bda, bda_str, 18));
            ESP_LOGD(TAG, "  - passkey: %lu", (uint32_t) param->key_notif.passkey);
            break;
        }

        default: {
            ESP_LOGW(TAG, "event: %d", event);
            break;
        }
    }

}
