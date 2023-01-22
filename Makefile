# Project Name
TARGET = DaisyHarmoniqs

# Sources
CPP_SOURCES = DaisyHarmoniqs.cpp

# Library Locations
LIBDAISY_DIR = ../libDaisy-4.0.0
DAISYSP_DIR = ../oma-git/DaisySP

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

