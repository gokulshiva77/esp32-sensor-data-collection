## ESP32 ADC Data Storage Example

This project demonstrates how to safely store and retrieve integer values (such as ADC readings) in non-volatile storage (NVS) on the ESP32 using FreeRTOS tasks and queues for thread safety.

### Key Components

- **storage_handler.c / storage_handler.h**: Implements a thread-safe storage handler using a FreeRTOS queue and a dedicated handler task. Provides APIs to write and read integer values to/from NVS in a specified namespace and key.
    - `storage_handler_write(namespace, key, value)`: Queues a write request to NVS.
    - `storage_handler_read(namespace, key, &value)`: Queues a read request from NVS.
    - `storage_handler_start()`: Initializes the handler task and queue. Call this once in `app_main` before using the handler.
- **adc_wrapper.c / adc_wrapper.h**: Provides a simple abstraction for ADC (Analog-to-Digital Converter) operations on the ESP32. It allows initialization, reading, and deinitialization of ADC units and channels using the ESP-IDF ADC oneshot driver. Includes validation for unit/channel and error handling with logging.
    - `adc_init(unit, channel)`: Initializes the specified ADC unit and channel.
    - `adc_read_channel(unit, channel)`: Reads the raw ADC value from the specified unit and channel.
    - `adc_deinit(unit, channel)`: Deinitializes the specified ADC channel and, if no channels remain, the unit.
    - `validate_adc_channel(unit, channel)`: Checks if the given unit and channel are valid for the ESP32.
- **main.c**: Application entry point. Sets up and starts the storage handler, ADC sampling, and a simple GPIO blink task using FreeRTOS. The ADC task periodically reads an ADC value, stores it in NVS using the storage handler, and reads it back for verification. The blink task toggles a GPIO pin (e.g., for an LED) every second.
    - `app_main()`: Initializes the storage handler and starts the ADC and blink tasks.
    - `adc_task()`: Reads ADC values, stores them in NVS, and reads them back, logging results.
    - `blink_task()`: Blinks an LED on a specified GPIO pin using FreeRTOS delays.

### How It Works

- All NVS operations are performed in a single handler task to ensure thread safety.
- Other tasks communicate with the handler via a FreeRTOS queue, sending read/write requests.
- The handler notifies the requesting task of the operation result using task notifications.
- The NVS namespace and key are user-defined strings (e.g., `adc_ns`, `adc_val`).

### Usage

1. Call `storage_handler_start()` once at startup.
2. Use `storage_handler_write()` and `storage_handler_read()` from any task to store or retrieve integer values in NVS.

### Error Handling

- All NVS and queue operations are logged. Errors are reported with ESP-IDF error codes for easy debugging.

### Requirements

- ESP-IDF 5.4.1 (ESP32 development framework)
- FreeRTOS (included in ESP-IDF)

---

### Hardware Used

- ESP32 WROOM 32 Kit
![ESP32 WROOM](images/ESP32.jpg)
---

### Pinout

![ESP32 WROOM Pinout](images/ESP32Pinout.png)

---
For more details, see the comments in `main/storage_handler.c` and ESP-IDF documentation on NVS and FreeRTOS.

### Issues Faced and Solutions in Linux

#### Flash Issues
- Run: sudo usermod -aG dialout $USER
- Run: `lsusb` to see if your FTDI device appears.
`Bus 001 Device 003: ID 10c4:ea60 Silicon Labs CP210x UART Bridge`
   - If not, unplug/replug and try a different USB port/cable.
   - If it appears, but OpenOCD still fails, you may need a udev rule:
     - Create a file `/etc/udev/rules.d/99-ftdi.rules` with:
       ```
       SUBSYSTEM=="usb", ATTR{idVendor}=="10c4", ATTR{idProduct}=="ea60", MODE="0666"
       ```
     - Then run:
       ```bash
       sudo udevadm control --reload-rules
       sudo udevadm trigger
       ```
     - Unplug and replug your debugger.