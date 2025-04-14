#ifndef MQTT_HPP
#define MQTT_HPP

#include <functional>
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include <map>
#include <string>
#include <freertos/FreeRTOS.h>
#include <ArduinoJson.h>
#include <variant>


namespace ez {
    namespace mqtt {

        class Client {
            public:
                using FuncVoid = std::function<void(void)>;
                using FuncJson = std::function<void(const JsonDocument&)>;
                using ReceiveHandler = std::variant<FuncVoid, FuncJson>;

                Client(const char* brokerUri) {
                    const esp_mqtt_client_config_t mqtt_cfg = {
                        .broker {
                            .address {
                                .uri = brokerUri,
                            }
                        },
                    };

                    clientHandle = esp_mqtt_client_init(&mqtt_cfg);
                }

                void subscribe(const char* topic, ReceiveHandler handler, int qos = 0) {
                    subscriptions.emplace(topic, Subscription{handler, qos});
                }

                int publish(const char* topic, std::string_view data, int qos = 0, int retain = 0) {
                    if (qos == 0) {
                        return esp_mqtt_client_publish(clientHandle, topic, data.data(), data.size(), qos, retain);
                    } else {
                        return esp_mqtt_client_enqueue(clientHandle, topic, data.data(), data.size(), qos, retain, false);
                    }
                }

                void start() {
                    esp_mqtt_client_register_event(clientHandle, MQTT_EVENT_ANY, event_handler, this);
                    esp_mqtt_client_start(clientHandle);
                }

            private:
                static void log_error_if_nonzero(const char *message, int error_code)
                {
                    if (error_code != 0) {
                        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
                    }
                }

                static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
                {
                    Client* self = static_cast<Client*>(handler_args);

                    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
                    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

                    switch ((esp_mqtt_event_id_t)event_id) {
                    case MQTT_EVENT_CONNECTED:
                        for (const auto& [topic, sub]: self->subscriptions) {
                            esp_mqtt_client_subscribe(self->clientHandle, topic.c_str(), sub.qos);
                        }
                        break;
                    case MQTT_EVENT_DISCONNECTED:
                        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
                        break;
                    case MQTT_EVENT_SUBSCRIBED:
                        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
                        break;
                    case MQTT_EVENT_UNSUBSCRIBED:
                        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
                        break;
                    case MQTT_EVENT_PUBLISHED:
                        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
                        break;
                    case MQTT_EVENT_DATA:
                        ESP_LOGI(TAG, "MQTT_EVENT_DATA. topic: %.*s", event->topic_len, event->topic);
                        self->handleData({event->topic, static_cast<size_t>(event->topic_len)}, {event->data, static_cast<size_t>(event->data_len)});
                        break;
                    case MQTT_EVENT_ERROR:
                        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
                        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
                        }
                        break;
                    default:
                        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
                        break;
                    }
                }
            private:
                void handleData(std::string_view topic, std::string_view data) {
                    if (auto it = subscriptions.find(std::string{topic}); 
                        it != subscriptions.end()) {
                        try {
                            std::visit([data](auto&& arg) {
                                using T = std::decay_t<decltype(arg)>;
                                if constexpr (std::is_same_v<T, FuncVoid>) {
                                    arg();
                                } 
                                else if constexpr (std::is_same_v<T, FuncJson>) {
                                    JsonDocument doc;
                                    if (DeserializationError err = deserializeJson(doc, data); err) {
                                        throw std::runtime_error(err.c_str());
                                    }
                                    arg(doc);
                                } 
                                else
                                    static_assert(false, "non-exhaustive visitor!");
                            }, it->second.handler);
                        } 
                        catch (const std::exception& e) {
                            ESP_LOGE(TAG, "EXCEPTION: %s", e.what());
                            publish("sensei/error", e.what(), 2);
                        }
                    }
                }

                struct Subscription {
                    ReceiveHandler handler;
                    int qos{0};
                };

                static constexpr char* TAG = "mqtt_client";
                esp_mqtt_client_handle_t clientHandle;
                std::map<std::string, Subscription> subscriptions;
        };
    }
}

#endif