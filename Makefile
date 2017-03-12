PROJECT ::= nfc-esp8266
SOURCES ::= src/main.cpp lib/mfrc522/src/MFRC522.cpp \
    lib/base64/src/base64encode.c
build.f_cpu ::= 80000000L
build.flash_ld ::= eagle.flash.4m1m.ld

build.project_name ::= $(PROJECT)

ARDUINO_HOME ?= $(HOME)/Arduino
TOOL_HOME ?= $(ARDUINO_HOME)/hardware/esp8266com/esp8266

OBJECTS ::= $(foreach s,$(SOURCES),$(s).o)
CORE_DIR ::= $(TOOL_HOME)/cores/esp8266
CORE_SOURCES ::= $(foreach dir,$(wildcard $(CORE_DIR)/*.c* $(CORE_DIR)/*/*.c* $(CORE_DIR)/*.S),$(subst $(TOOL_HOME)/,,$(dir)))
CORE_OBJECTS ::= $(CORE_SOURCES:.c=.c.o)
CORE_OBJECTS ::= $(CORE_OBJECTS:.cpp=.cpp.o)
CORE_OBJECTS ::= $(CORE_OBJECTS:.S=.S.o)
CORE_OBJECTS ::= $(filter-out %.unused,$(CORE_OBJECTS))

LIBRARY_DIR ::= $(TOOL_HOME)/libraries
LIBRARY_SOURCES ::= $(foreach dir,$(wildcard $(LIBRARY_DIR)/*/*.c* $(LIBRARY_DIR)/*/src/*.c*),$(subst $(TOOL_HOME)/,,$(dir)))
LIBRARY_OBJECTS ::= $(LIBRARY_SOURCES:.c=.c.o)
LIBRARY_OBJECTS ::= $(LIBRARY_OBJECTS:.cpp=.cpp.o)
# This file doesn't handle the ARDUINO_BOARD macro correctly
LIBRARY_OBJECTS ::= $(filter-out %/ESP8266mDNS.cpp.o,$(LIBRARY_OBJECTS))

runtime.platform.path ::= $(TOOL_HOME)
build.board ::= nodemcuv2
runtime.ide.version ::= 10801
includes ::= -I$(TOOL_HOME)/cores/esp8266 -I$(TOOL_HOME)/variants/nodemcu \
    $(foreach dir,$(wildcard $(TOOL_HOME)/libraries/* $(TOOL_HOME)/libraries/*/src),-I$(dir)) \
    -I$(TOOL_HOME)/libraries/SPI \
    $(foreach dir,$(wildcard lib/*/src),-I$(dir))
build.path ::= build

build/$(PROJECT).hex: build/$(PROJECT).elf
	$(eval build.flash_mode ::= dio)
	$(eval build.flash_freq ::= 40)
	$(eval build.flash_size ::= 4M) 
	$(recipe.objcopy.hex.pattern)

build/$(PROJECT).elf: $(foreach obj,$(OBJECTS),build/$(obj)) build/arduino.ar
	$(eval object_files ::= $^)
	$(recipe.c.combine.pattern)

# object_file is quoted, so the members must be added individually
define add_archive_member =
  $(eval object_file ::= $(1))
  $(recipe.ar.pattern)
endef

# TODO: This doesn't delete arduino.ar on failure
build/arduino.ar: $(foreach obj,$(CORE_OBJECTS),build/$(obj)) $(foreach obj,$(LIBRARY_OBJECTS),build/$(obj))
	$(foreach obj,$^,$(call add_archive_member,$(obj)))

build/%.c.o: %.c build platformconfig
	-mkdir --parents build/$(shell dirname $(<))
	$(eval source_file ::= $<)
	$(eval object_file ::= $@)
	$(recipe.c.o.pattern)

build/%.cpp.o: %.cpp build platformconfig
	-mkdir --parents build/$(shell dirname $(<))
	$(eval source_file ::= $<)
	$(eval object_file ::= $@)
	$(recipe.cpp.o.pattern)

# Build core and libraries
build/%.cpp.o: $(TOOL_HOME)/%.cpp build platformconfig
	-mkdir --parents $(dir $(@:.o=))
	$(eval source_file ::= $<)
	$(eval object_file ::= $@)
	$(recipe.cpp.o.pattern)

build/%.c.o: $(TOOL_HOME)/%.c build platformconfig
	-mkdir --parents $(dir $(@:.o=))
	$(eval source_file ::= $<)
	$(eval object_file ::= $@)
	$(recipe.c.o.pattern)

build/%.S.o: $(TOOL_HOME)/%.S build platformconfig
	-mkdir --parents $(dir $(@:.o=))
	$(eval source_file ::= $<)
	$(eval object_file ::= $@)
	$(recipe.S.o.pattern)

build:
	-mkdir build

clean:
	-rm -R build

# Includes the modified platform.txt
platformconfig: build/platform.mk
	$(eval include $<)

# Hack platform.txt so it uses Makefile variable substitution
build/platform.mk: build
	sed 's,{\([^}]*\)},$$(\1),g' < $(TOOL_HOME)/platform.txt > $@
	
.PHONY: platformconfig clean

