-include config.mk

LIB_DIR = ./lib
SRC_DIR = ./src

# If CROSS_COMPILE is defined, use it as a prefix for the compiler
CC = $(CROSS_COMPILE)gcc

# List all .c files from SRC_DIR except hook.c
SRC_FILES = $(filter-out $(SRC_DIR)/hook.c, $(wildcard $(SRC_DIR)/*.c))
# Create corresponding .o filenames
OBJ_FILES = $(SRC_FILES:.c=.o)
# Define the output shared library for rdma_client
LIBRARY_RDMA_CLIENT = $(LIB_DIR)/librdma_client.so
# Define the output shared library for hook
LIBRARY_HOOK = $(LIB_DIR)/libhook.so

# Define linker flags variable based on CUSTOM_LIB_PATH
LDFLAGS := $(if $(CUSTOM_LIB_PATH),-L$(CUSTOM_LIB_PATH),)

# Add the -fPIC option to CFLAGS
CFLAGS += -fPIC

.PHONY: all clean

all: $(LIBRARY_RDMA_CLIENT) $(LIBRARY_HOOK)

# Rule to build the rdma_client shared library
$(LIBRARY_RDMA_CLIENT): $(OBJ_FILES)
	mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $(OBJ_FILES) -ldl -lrdmacm

# Rule to build the hook shared library
$(LIBRARY_HOOK): $(SRC_DIR)/hook.o $(LIBRARY_RDMA_CLIENT)
	mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $(SRC_DIR)/hook.o -ldl -L$(LIB_DIR) -lrdma_client -lrdmacm

# Rule to build object files
%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(LIB_DIR)
	rm -f $(SRC_DIR)/*.o
