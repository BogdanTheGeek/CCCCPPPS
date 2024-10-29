all : flash

TARGET:=main
CH32V003FUN ?= ../ch32v003fun/ch32v003fun

ADDITIONAL_C_FILES += ./rv003usb/rv003usb.S ./rv003usb/rv003usb.c
ADDITIONAL_C_FILES += $(filter-out $(TARGET).c, $(wildcard *.c))
EXTRA_CFLAGS:=-I./lib -I./rv003usb

include $(CH32V003FUN)/ch32v003fun.mk

flash : cv_flash
clean : cv_clean

