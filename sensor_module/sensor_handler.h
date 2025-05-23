#ifndef __SENSOR_HANDLER_H__
#define __SENSOR_HANDLER_H__

typedef int (*sensor_read_func_t)(void);

void sensor_init(void);

void sensor_deinit(void);

#endif // __SENSOR_HANDLER_H__