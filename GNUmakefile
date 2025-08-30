# Copyright (c) 2025 Mamadou Babaei
#
# Author: Mamadou Babaei <info@babaei.net>
#


DIR_ROOT	:=	$(CURDIR)
DIR_SRC		:=	$(DIR_ROOT)/src
DIR_ASM		:=	$(DIR_ROOT)/asm
DIR_BIN		:=	$(DIR_ROOT)/bin

CXX		:=	clang++
CXXFLAGS	:=	-std=c++23 -march=native -O3 -fno-rtti -fno-exceptions -pthread
CXXFLAGS_ASM	:=	$(CXXFLAGS) -S -masm=intel -fverbose-asm

BINARIES	:=	bench-dod \
			bench-dod-double \
			bench-dod-avx2 \
			bench-dod-avx2-double \
			bench-dod-znver2 \
			bench-dod-znver2-double \
			bench-repository \
			bench-repository-double

ASM_FILES	:=	$(addprefix $(DIR_ASM)/,$(addsuffix .s,$(BINARIES)))

.PHONY: all
all: $(addprefix $(DIR_BIN)/,$(BINARIES)) $(ASM_FILES)

$(DIR_BIN)/%: $(DIR_SRC)/%.cpp
	@echo "Building $(subst $(DIR_ROOT)/,,$@)..."
	@mkdir -p "$(DIR_BIN)"
	@$(CXX) $(CXXFLAGS) -o $@ $<

$(DIR_ASM)/%.s: $(DIR_SRC)/%.cpp
	@echo "Generating assembly code $(subst $(DIR_ROOT)/,,$@)..."
	@mkdir -p "$(DIR_ASM)"
	@$(CXX) $(CXXFLAGS_ASM) -o $@ $<

.PHONY: clean
clean:
	@echo "Cleaning up..."
	@rm -rf "$(DIR_ASM)"
	@rm -rf "$(DIR_BIN)"
	@echo "Cleaning up successfully!"
