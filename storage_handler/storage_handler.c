#include "storage_handler.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define STORAGE_TAG "STORAGE_HANDLER"

// Thread-safe handler task and queue
// Message types for storage operations
typedef enum {
    STORAGE_OP_WRITE,
    STORAGE_OP_READ
} storage_op_t;

typedef struct {
    storage_op_t op;
    const char *namespace_name;
    const char *key;
    int32_t value; // For write
    int32_t *read_result; // For read
    TaskHandle_t requester; // To notify the requesting task
    esp_err_t result;
} storage_msg_t;

#define STORAGE_QUEUE_LENGTH 4
static QueueHandle_t storage_queue = NULL;

// Private function declarations
esp_err_t storage_init(void);
esp_err_t storage_write_int(const char *namespace_name, const char *key, int32_t value);
esp_err_t storage_read_int(const char *namespace_name, const char *key, int32_t *out_value);


// Direct, non-thread-safe NVS APIs
esp_err_t storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS Flash init failed: %s", __func__, esp_err_to_name(err));
    }
    return err;
}

esp_err_t storage_write_int(const char *namespace_name, const char *key, int32_t value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS open failed: %s", __func__, esp_err_to_name(err));
        return err;
    }
    err = nvs_set_i32(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS set failed: %s", __func__, esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS commit failed: %s", __func__, esp_err_to_name(err));
    }
    nvs_close(handle);
    return err;
}

esp_err_t storage_read_int(const char *namespace_name, const char *key, int32_t *out_value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS open failed: %s", __func__, esp_err_to_name(err));
        return err;
    }
    err = nvs_get_i32(handle, key, out_value);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS get failed: %s", __func__, esp_err_to_name(err));
    }
    nvs_close(handle);
    return err;
}

static void storage_handler_task(void *pvParameter)
{
    storage_msg_t msg;
    esp_err_t init_err = storage_init(); // Initialize NVS here
    if (init_err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "%s NVS init failed: %s", __func__, esp_err_to_name(init_err));
        vTaskDelete(NULL);
    }
    while (1) {
        if (xQueueReceive(storage_queue, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(STORAGE_TAG, "%s Received message: op=%d, namespace=%s, key=%s", __func__, msg.op, msg.namespace_name, msg.key);
            if (msg.op == STORAGE_OP_WRITE) {
                msg.result = storage_write_int(msg.namespace_name, msg.key, msg.value);
                ESP_LOGI(STORAGE_TAG, "Write result: %s", esp_err_to_name(msg.result));
            } else if (msg.op == STORAGE_OP_READ) {
                msg.result = storage_read_int(msg.namespace_name, msg.key, msg.read_result);
                ESP_LOGI(STORAGE_TAG, "Read result: %s", esp_err_to_name(msg.result));
            }
            // Notify the requester (if any)
            if (msg.requester) {
                xTaskNotify(msg.requester, msg.result, eSetValueWithOverwrite);
            }
        }
    }
}

// API to queue a write request
esp_err_t storage_handler_write(const char *namespace_name, const char *key, int32_t value)
{
    storage_msg_t msg = {
        .op = STORAGE_OP_WRITE,
        .namespace_name = namespace_name,
        .key = key,
        .value = value,
        .requester = xTaskGetCurrentTaskHandle(),
        .result = ESP_FAIL
    };
    if (xQueueSend(storage_queue, &msg, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(STORAGE_TAG, "Failed to send write message to queue");
        return ESP_FAIL;
    }
    // Wait for notification from handler and get the result from notification value
    uint32_t notify_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    esp_err_t result = (esp_err_t)notify_value;
    ESP_LOGI(STORAGE_TAG, "storage_handler_write result: %s", esp_err_to_name(result));
    return result;
}

// API to queue a read request
esp_err_t storage_handler_read(const char *namespace_name, const char *key, int32_t *out_value)
{
    storage_msg_t msg = {
        .op = STORAGE_OP_READ,
        .namespace_name = namespace_name,
        .key = key,
        .read_result = out_value,
        .requester = xTaskGetCurrentTaskHandle(),
        .result = ESP_FAIL
    };
    if (xQueueSend(storage_queue, &msg, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(STORAGE_TAG, "Failed to send read message to queue");
        return ESP_FAIL;
    }
    // Wait for notification from handler and get the result from notification value
    uint32_t notify_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    esp_err_t result = (esp_err_t)notify_value;
    ESP_LOGI(STORAGE_TAG, "storage_handler_read result: %s", esp_err_to_name(result));
    return result;
}

// Call this once in app_main or before using the handler
void storage_handler_start(void)
{
    if (!storage_queue) {
        storage_queue = xQueueCreate(STORAGE_QUEUE_LENGTH, sizeof(storage_msg_t));
        xTaskCreate(storage_handler_task, "storage_handler_task", 2048, NULL, 5, NULL);
    }
}
