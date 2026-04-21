SHELL := /usr/bin/env bash

BUILD_DIR ?= build-chromack
CMAKE ?= cmake
JOBS ?= $(shell nproc)
INSTALL_PREFIX ?= $(HOME)/.local

.PHONY: compile configure install clean

configure:
	$(CMAKE) -S . -B $(BUILD_DIR)

compile: configure
	$(CMAKE) --build $(BUILD_DIR) -j$(JOBS)

install: compile
	$(CMAKE) --install $(BUILD_DIR) --prefix $(INSTALL_PREFIX)

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
