PROJECT_NAME = $(shell basename "$(realpath ./)")

APPLICATION_SRCS = $(notdir $(wildcard ./*.c))
#APPLICATION_SRCS += system_nrf51.c
APPLICATION_SRCS += softdevice_handler.c
APPLICATION_SRCS += ble_advdata.c
APPLICATION_SRCS += ble_conn_params.c
APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += ble_srv_common.c
# APPLICATION_SRCS += ble_db_discovery.c
# APPLICATION_SRCS += device_manager_central.c
# APPLICATION_SRCS += pstorage.c
APPLICATION_SRCS += nrf_delay.c
APPLICATION_SRCS += spi_slave.c
APPLICATION_SRCS += led.c

DEVICE = NRF51

ifndef TARGET
	TARGET = TORCH
endif

TARGET_UPPER = $(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LOWER = $(shell echo $(TARGET) | tr A-Z a-z)
BOARD = BOARD_$(TARGET_UPPER)

APPLICATION_SRCS += $(TARGET_LOWER).c

USE_SOFTDEVICE = s110

SDK_VERSION = 7

SDK_PATH ?= $(HOME)/code/nrf51822_sdk/sdk7/
SOFTDEVICE ?= $(HOME)/code/nrf51822_sdk/s110_nrf51822_7.1.0_softdevice.hex

TEMPLATE_PATH ?= ../../squall/software/nrf51-pure-gcc-setup/template/

LIBRARY_PATHS += ./
LIBRARY_PATHS += ../../squall/software/include
SOURCE_PATHS += ../../squall/software/src

CFLAGS = -Os
GDB_PORT_NUMBER = 2331

include $(TEMPLATE_PATH)Makefile
