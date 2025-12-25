all : flash

TARGET:=main
TARGET_MCU:=CH32V003
CH32FUN ?= ../ch32fun/ch32fun

ADDITIONAL_C_FILES += ../rv003usb/rv003usb.S ../rv003usb/rv003usb.c
ADDITIONAL_C_FILES += $(filter-out $(TARGET).c, $(wildcard *.c))
EXTRA_CFLAGS:=-I../lib -I../rv003usb

LDFLAGS+=-T nonvolatile.ld

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean

