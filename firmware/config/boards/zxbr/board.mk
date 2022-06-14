# List of all the board related files.
BOARDCPPSRC =  $(PROJECT_DIR)/config/boards/zxbr/board_configuration.cpp

BOARDINC = $(PROJECT_DIR)/config/boards/zxbr

# Override DEFAULT_ENGINE_TYPE
DDEFS += -DSHORT_BOARD_NAME=zxbr
DDEFS += -DFIRMWARE_ID=\"zxbr\"
DDEFS += -DDEFAULT_ENGINE_TYPE=MINIMAL_PINS
