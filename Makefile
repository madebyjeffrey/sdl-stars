.DEFAULT_GOAL := help
.PHONY: help all version-header text-header data-header text-unreferenced python-env save-cli compile host test-unit test-host test-precision precision precision-turns scenario run fmt compile-check res resources regression regression-quick regression-host regression-export tutorial tutorial-reject mac-bundle clean

DIST_DIR    ?= dist
CMAKE       ?= cmake
CLANG_FORMAT ?= clang-format
PYTHON      ?= python3
ifeq ($(OS),Windows_NT)
PYTHON_VENV = $(DIST_DIR)/$(HOST_PRESET)/python-venv/Scripts/python.exe
else
PYTHON_VENV = $(DIST_DIR)/$(HOST_PRESET)/python-venv/bin/python
endif
GO          ?= go
SAVE_CLI    := $(abspath $(DIST_DIR))/stars-save$(if $(filter Windows_NT,$(OS)),.exe,)
FORMAT_FILES ?= $(wildcard src/core/*.c src/core/*.h src/ui/*.c src/ui/*.h src/host/*.c src/platform/*.c src/platform/*.h res/*.h tests/scaffold/*.c tests/scaffold/*.h)
SEED        ?= 12345
# The native regression: a release build, its fixed-seed run, and the
# checked-in baseline it is compared against (tests/scaffold/REGRESSION.md).
REGRESSION_BUILD := $(DIST_DIR)/regression-build
REGRESSION_WORK  ?= $(DIST_DIR)/scaffold/regression/native
REGRESSION_REPORT := $(DIST_DIR)/scaffold/regression/comparison.json
BASELINE_DIR     := tests/scaffold/fixtures/regression/native
# Every scenario in the baseline, unless SCENARIOS names some.
SCENARIOS ?= $(shell $(PYTHON) -c "import json; print(' '.join(json.load(open('$(BASELINE_DIR)/run.json'))['scenarios']))")
THROUGH   ?= 150
REGRESSION_ARGS = $(foreach s,$(SCENARIOS),--scenario $(s)) --through $(THROUGH)
# Select the SDL product preset for this development machine.
ifeq ($(OS),Windows_NT)
CLIENT_PRESET ?= windows-sdl-debug
CLIENT_RELEASE_PRESET ?= windows-sdl-release
EXE_SUFFIX := .exe
else ifeq ($(shell uname -s),Darwin)
CLIENT_PRESET ?= macos-sdl-debug
CLIENT_RELEASE_PRESET ?= macos-sdl-release
EXE_SUFFIX :=
else
CLIENT_PRESET ?= sdl-debug
CLIENT_RELEASE_PRESET ?= sdl-release
EXE_SUFFIX :=
endif
HOST_PRESET ?= host-release
test-unit regression tutorial scenario: export SDL_VIDEODRIVER := dummy
test-unit regression tutorial scenario: export SDL_AUDIODRIVER := dummy
test-unit regression tutorial scenario: export STARS_SDL_RENDERER := software
test-unit regression tutorial scenario: export STARS_FILE_DIALOG_BACKEND := compat
HOST_WORK   ?= $(DIST_DIR)/scaffold/regression/host

help:
	@echo "Targets:"
	@echo "  save-cli             Build the standalone test save CLI"
	@echo "  compile              Build the SDL desktop client (CLIENT_PRESET=$(CLIENT_PRESET))"
	@echo "  host                 Build stars-host with the native compiler (HOST_PRESET=$(HOST_PRESET))"
	@echo "  test-unit            Build and run the SDL game unit tests"
	@echo "  test-host            Build and run the unit tests that need no windows, natively"
	@echo "  precision            Measure extended versus double intermediates (PRECISION_ARGS to tune)"
	@echo "  precision-turns      Compare real turns from two temporary host builds (PRECISION_TURN_ARGS)"
	@echo "  test-precision       Check the precision experiment's coverage, witnesses and reports"
	@echo "  scenario             Build a test game into dist/scenarios/SCENARIO (no SCENARIO: list them)"
	@echo "  fmt                  Format C sources and headers (FORMAT_FILES=src/core/ai.c to limit)"
	@echo "  compile-check        Compile the SDL client and generated resources"
	@echo "  python-env           Configure the host preset and set up its Python venv"
	@echo "  text-header/data-header Generate text and game-data tables"
	@echo "  text-unreferenced    List text rows that no code names and no @range covers"
	@echo "  res / resources      Build the generated SDL resources"
	@echo "  regression           Run the native regression and compare it with the baseline"
	@echo "                       (SCENARIOS=\"noai smallai4\" and THROUGH=10 to limit)"
	@echo "  regression-quick     Run smallai4 through turn 10 and compare"
	@echo "  regression-host      Run the native regression with stars-host instead of stars-sdl"
	@echo "  regression-export    Replace the baseline with the last full regression run"
	@echo "  tutorial             Run the available SDL tutorial checkpoints (partial coverage)"
	@echo "  tutorial-reject      Verify early Generate is rejected"
	@echo "  mac-bundle           Build and sign Stars!.app using an auto-discovered keychain identity"
	@echo "  clean                Remove $(DIST_DIR)/"

mac-bundle:
	$(CMAKE) --preset macos-bundle
	$(CMAKE) --build --preset macos-bundle

all: compile

save-cli:
	@mkdir -p "$(DIST_DIR)"
	cd tests/savecli && $(GO) build -o "$(SAVE_CLI)" .

compile:
	$(CMAKE) --preset $(CLIENT_PRESET)
	$(CMAKE) --build --preset $(CLIENT_PRESET)

test-unit: compile
	cd "$(DIST_DIR)/$(CLIENT_PRESET)" && ctest --output-on-failure --timeout 300 -R "^sdl_test_" $(CTEST_ARGS)

# The unit tests that need no windows, built with the host preset and run
# without Wine.
test-host: host
	cd "$(DIST_DIR)/$(HOST_PRESET)" && ctest --output-on-failure --timeout 300 $(CTEST_ARGS)

# Temporary source-extracted copies; production arithmetic and baselines stay put.
precision: host
	$(PYTHON) tests/scaffold/precision.py --generated-dir "$(DIST_DIR)/$(HOST_PRESET)/generated" $(PRECISION_ARGS)

precision-turns: save-cli
	$(PYTHON) tests/scaffold/precision_turns.py --cli "$(SAVE_CLI)" $(PRECISION_TURN_ARGS)

test-precision: host
	STARS_PRECISION_GENERATED_DIR="$(abspath $(DIST_DIR))/$(HOST_PRESET)/generated" $(PYTHON) -m unittest discover -s tests/scaffold -p test_precision.py

# SCENARIO names one of the games in tests/scenarios/scenarios.c.
SCENARIO_DIR = $(abspath $(DIST_DIR))/scenarios/$(SCENARIO)
scenario: compile
ifeq ($(SCENARIO),)
	cd "$(DIST_DIR)/$(CLIENT_PRESET)/tests-client" && ./stars_scenario$(EXE_SUFFIX)
else
	rm -rf "$(SCENARIO_DIR)"
	mkdir -p "$(SCENARIO_DIR)"
	cd "$(DIST_DIR)/$(CLIENT_PRESET)/tests-client" && ./stars_scenario$(EXE_SUFFIX) "$(SCENARIO)" "$(SCENARIO_DIR)"
endif

run: compile
	"$(DIST_DIR)/$(CLIENT_PRESET)/bin/stars-sdl$(EXE_SUFFIX)"

fmt:
	$(CLANG_FORMAT) --style=file -i $(FORMAT_FILES)

# Direct compiles need the generated version.h, text and data tables that
# CMake builds normally make.
VERSION_DIR := $(abspath $(DIST_DIR))/generated
text-header:
	$(MAKE) python-env
	@$(PYTHON_VENV) text/textgen.py "$(VERSION_DIR)"

data-header:
	$(MAKE) python-env
	@$(PYTHON_VENV) data/datagen.py "$(VERSION_DIR)"

python-env:
	$(CMAKE) --preset $(HOST_PRESET)

text-unreferenced:
	@$(PYTHON) text/textgen.py --unreferenced

version-header:
	@mkdir -p "$(VERSION_DIR)"
	@$(CMAKE) -DSOURCE_DIR="$(CURDIR)" -DOUTPUT="$(VERSION_DIR)/version.h" \
		-DVERSION_BASE=$$(sed -n 's/^project(stars VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt) \
		-P cmake/version.cmake

compile-check: compile

res: resources
resources: compile

# Builds the release exe as CI does, runs a fresh work directory with
# stars-sdl -s$(SEED) and compares it with the baseline. A difference exits
# nonzero.
regression: save-cli
	$(CMAKE) --preset $(CLIENT_RELEASE_PRESET) -B "$(REGRESSION_BUILD)"
	$(CMAKE) --build "$(REGRESSION_BUILD)"
	rm -rf "$(REGRESSION_WORK)"
	$(PYTHON) tests/scaffold/regression.py prepare --seed $(SEED) --exe "$(REGRESSION_BUILD)/bin/stars-sdl$(EXE_SUFFIX)" --work "$(REGRESSION_WORK)"
	$(PYTHON) tests/scaffold/regression.py run --cli "$(SAVE_CLI)" --work "$(REGRESSION_WORK)" $(REGRESSION_ARGS)
	$(PYTHON) tests/scaffold/regression.py compare "$(BASELINE_DIR)" "$(REGRESSION_WORK)" $(REGRESSION_ARGS) --report "$(REGRESSION_REPORT)"

regression-quick:
	$(MAKE) regression SCENARIOS=smallai4 THROUGH=10

host:
	$(CMAKE) --preset $(HOST_PRESET)
	$(CMAKE) --build --preset $(HOST_PRESET)

# The same run and baseline as `make regression`, through stars-host and
# without Wine.
regression-host: save-cli host
	rm -rf "$(HOST_WORK)"
	$(PYTHON) tests/scaffold/regression.py prepare --seed $(SEED) --exe "$(DIST_DIR)/$(HOST_PRESET)/bin/stars-host$(EXE_SUFFIX)" --work "$(HOST_WORK)"
	$(PYTHON) tests/scaffold/regression.py run --cli "$(SAVE_CLI)" --work "$(HOST_WORK)" $(REGRESSION_ARGS)
	$(PYTHON) tests/scaffold/regression.py compare "$(BASELINE_DIR)" "$(HOST_WORK)" $(REGRESSION_ARGS) --report "$(DIST_DIR)/scaffold/regression/host-comparison.json"

# For a commit that changes behavior on purpose: after a full `make
# regression`, replace the baseline with that run.
regression-export:
	$(PYTHON) tests/scaffold/regression.py export --work "$(REGRESSION_WORK)" $(foreach s,$(SCENARIOS),--scenario $(s)) --replace

# These checkpoints are bounded; full 80-page SDL coverage is still pending.
tutorial: compile
	cd "$(DIST_DIR)/$(CLIENT_PRESET)" && ctest --output-on-failure -R "win32_native_tutorial_.*dummy$$" $(CTEST_ARGS)

tutorial-reject: tutorial

clean:
	rm -rf "$(DIST_DIR)"
