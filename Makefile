
LIBNAME    = libosdlp.a
QA_EXE     = test_osdlp

SRC_DIR    = src
QA_SRC_DIR = test
INCL_DIR   = include
DEPS       = ${INCL_DIR}/osdlp.h

SRC        = $(wildcard $(SRC_DIR)/*.c)
OBJ        = $(SRC:$(SRC_DIR)/%.c=$(SRC_DIR)/%.o)
QA_SRC     = $(wildcard $(QA_SRC_DIR)/*.c)

INCLUDES   += -I$(INCL_DIR)
QA_INC     = $(INCLUDES)
QA_INC     += -I$(QA_SRC_DIR)
CFLAGS     += -Wall -g
LDLIBS     += 
QA_LDLIBS  += -lcmocka

all: $(QA_EXE)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INCL_DIR)/%.h
	$(CC) $(INCLUDES) $(CFLAGS) $(LDLIBS) -c -o $@ $<

$(LIBNAME): $(OBJ) $(DEPS)
	${AR} ru $@ $^

$(QA_EXE): $(LIBNAME) $(QA_SRC)
	$(CC) $(QA_INC) $(LDFLAGS) $^ $(QA_LDLIBS) ${LIBNAME} -o $@
	
.PHONY: test
test: $(QA_EXE)
	./$(QA_EXE)

clean:
	$(RM) $(OBJ)
	$(RM) $(QA_EXE)
	$(RM) $(LIBNAME)
