# Makefile para Discord RPC C++

CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Iinclude

SRC_DIR := src
OBJ_DIR := build
BIN := test

# todos los .cpp
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all linux windows clean

all:
	@echo "Use: make linux or make windows"

# ---------------- Linux ----------------
linux: $(BIN)

# link final
$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# compilar cada objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# crear build si no existe
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ---------------- Windows ----------------
windows:
	@echo "Not implemented"

# ---------------- Clean ----------------
clean:
	rm -rf $(OBJ_DIR) $(BIN)
