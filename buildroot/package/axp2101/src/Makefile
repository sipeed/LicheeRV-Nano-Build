APP_DIR = ./src
SRC_DIR = ./XPowersLib
DIR_BIN = ./bin

OBJ_C = $(wildcard ${SRC_DIR}/*.cpp ${APP_DIR}/*.cpp)
OBJ_O = $(patsubst %.cpp,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

TARGET = axp2101

$(shell mkdir -p $(DIR_BIN))

MSG = -g -O0 -Wall 
CFLAGS += $(MSG)

${TARGET}:${OBJ_O}
	$(CXX) $(CFLAGS) $(OBJ_O) -o $@ $(LIB)

${DIR_BIN}/%.o:$(APP_DIR)/%.cpp
	$(CXX) $(CFLAGS) -c  $< -o $@ -I $(SRC_DIR)

${DIR_BIN}/%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CFLAGS) -c  $< -o $@ $(LIB)

clean :
	rm $(DIR_BIN)/*.* 
	rm $(TARGET) 
