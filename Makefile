SHELL := /usr/bin/env bash

BUILD_DIR ?= build-chromack
CMAKE ?= cmake
JOBS ?= $(shell nproc)

.PHONY: compile configure clean

configure:
	$(CMAKE) -S . -B $(BUILD_DIR)

compile: configure
	$(CMAKE) --build $(BUILD_DIR) -j$(JOBS)

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
