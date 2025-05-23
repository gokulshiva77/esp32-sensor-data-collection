#ifndef STORAGE_HANDLER_H
#define STORAGE_HANDLER_H

#include <stdint.h>
#include <esp_err.h>
#include <esp_log.h>


// Start the storage handler task and queue
void storage_handler_start(void);

// Thread-safe write and read APIs using the handler
esp_err_t storage_handler_write(const char *namespace_name, const char *key, int32_t value);
esp_err_t storage_handler_read(const char *namespace_name, const char *key, int32_t *out_value);


#endif // STORAGE_HANDLER_H
