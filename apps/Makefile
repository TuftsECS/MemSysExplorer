# Setting Application Build Settings
APPS_HOME ?= $(shell readlink -f "$(CURDIR)")
PATCH_DIR := $(APPS_HOME)/patches
ENV_DIR := $(APPS_HOME)/tools
JSON_FILE := $(APPS_HOME)/built_profilers.json

# Define profiler directories
DYNAMORIO_DIR := $(APPS_HOME)/profilers/dynamorio
SNIPER_DIR := $(APPS_HOME)/profilers/sniper
NCU_DIR := $(APPS_HOME)/profilers/ncu
NVBIT_DIR := $(APPS_HOME)/profilers/nvbit
PERF_DIR := $(APPS_HOME)/profilers/perf
DOCS_DIR := $(APPS_HOME)/docs

.PHONY: all dynamorio sniper nvbit perf ncu docs clean clean_docs

# Default target: build everything
all: dynamorio sniper nvbit perf ncu docs
	@echo "All profilers and documentation built successfully."

# Initialize JSON file if it doesn't exist
init_json:
	@if [ ! -f "$(JSON_FILE)" ]; then \
		echo '{ "dynamorio": false, "ncu": false, "nvbit": false, "sniper": false, "perf": false }' > "$(JSON_FILE)"; \
	fi

# Update JSON file when a profiler is built
update_json:
	@jq --arg key "$(PROFILER)" '.[$$key] = true' "$(JSON_FILE)" > "$(JSON_FILE).tmp" && mv "$(JSON_FILE).tmp" "$(JSON_FILE)"

dynamorio: init_json
	@echo "            Building DynamoRIO             "
	cd $(DYNAMORIO_DIR) && make PATCH_DIR=$(PATCH_DIR)
	@echo "DynamoRIO build completed."
	@make update_json PROFILER=dynamorio

ncu: init_json
	@echo "               Checking CUDA               "
	cd $(NCU_DIR) && make check_all APPS_HOME=$(APPS_HOME)
	@echo "Nsight Profilers check completed."
	@make update_json PROFILER=ncu

sniper: init_json
	@echo "              Building Sniper              "
	cd $(SNIPER_DIR) && make PATCH_DIR=$(PATCH_DIR)
	@echo "Sniper build completed."
	@make update_json PROFILER=sniper

nvbit: init_json
	@echo "              Building NVBit               "
	cd $(NVBIT_DIR) && make PATCH_DIR=$(PATCH_DIR)
	@echo "NVBit build completed."
	@make update_json PROFILER=nvbit

perf: init_json
	@echo "              Building Perf                "
	@echo "Perf is completed."
	@make update_json PROFILER=perf

docs:
	@echo "        Building Project Documentation      "
	cd $(APPS_HOME) && sphinx-apidoc -o $(DOCS_DIR)/source/ ./ --force --no-toc -e && cd $(DOCS_DIR) && make html
	@echo "Documentation built at $(DOCS_DIR)/_build/html"

# Clean all profilers and docs
clean: clean_dynamorio clean_sniper clean_nvbit clean_perf clean_docs clean_pycache
	@rm -f $(JSON_FILE)
	@echo "         Cleaning Completed                "

# Clean individual profilers
clean_dynamorio:
	@echo "Cleaning DynamoRIO..."
	cd $(DYNAMORIO_DIR) && make clean
	@echo "DynamoRIO cleaned."

clean_sniper:
	@echo "Cleaning Sniper..."
	cd $(SNIPER_DIR) && make clean
	@echo "Sniper cleaned."

clean_nvbit:
	@echo "Cleaning NVBit..."
	cd $(NVBIT_DIR) && make clean
	@echo "NVBit cleaned."

clean_perf:
	@echo "Cleaning Perf..."
	@echo "Perf working directory is cleared."

# Clean __pycache__ directories
clean_pycache:
	@echo "      Removing caching directories         "
	find $(APPS_HOME) -type d -name "__pycache__" -exec rm -rf {} +
	@echo "All __pycache__ directories removed."

clean_docs:
	@echo "Cleaning documentation build directory..."
	rm -rf $(DOCS_DIR)/build && rm -rf $(DOCS_DIR)/source/*.rst
	@echo "Documentation build directory removed."
