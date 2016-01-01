CC = g++
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = ../obj
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/CCommandTest

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/CCommandTest

SRC = \
CCommandTest.cpp \

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
-std=c++11 \
-I$(INC_DIR) \
-I../../CReadLine/include \
-I../../CStrUtil/include \
-I../../CFile/include \
-I../../CUtil/include \

LFLAGS = \
$(LEBUG) \
-L$(LIB_DIR) \
-L../../CReadLine/lib \
-L../../CStrUtil/lib \
-L../../CFile/lib \
-L../../COS/lib \
-lCCommand -lCReadLine -lCFile -lCStrUtil -lCOS \
-lreadline -lcurses

.SUFFIXES: .cpp

$(OBJS): $(OBJ_DIR)/%.o: %.cpp
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/CCommandTest: $(OBJS)
	$(CC) -o $(BIN_DIR)/CCommandTest $(OBJS) $(LFLAGS) -ltre
