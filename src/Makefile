CC = g++
AR = ar
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = ../obj
LIB_DIR = ../lib

all: $(LIB_DIR)/libCCommand.a

SRC = \
CCommandMgr.cpp \
CCommand.cpp \
CCommandDest.cpp \
CCommandFileDest.cpp \
CCommandFileSrc.cpp \
CCommandPipe.cpp \
CCommandPipeDest.cpp \
CCommandPipeSrc.cpp \
CCommandSrc.cpp \
CCommandStringDest.cpp \
CCommandStringSrc.cpp \
CCommandUtil.cpp \

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
-std=c++11 \
-I. -I$(INC_DIR) \
-I../../CUtil/include \
-I../../CFile/include \
-I../../COS/include \
-I../../CStrUtil/include \

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(LIB_DIR)/libCCommand.a

$(OBJS): $(OBJ_DIR)/%.o: %.cpp
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

.SUFFIXES: .cpp

$(LIB_DIR)/libCCommand.a: $(OBJS)
	$(AR) crv $(LIB_DIR)/libCCommand.a $(OBJS)
