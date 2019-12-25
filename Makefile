EXE = out

SRC_DIR = Src
#OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(SRC_DIR)/%.o)

CPPFLAGS += -IInc
CFLAGS += -Wall -g
LDLIBS += -lcmocka

all: $(EXE) clean

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	
.PHONY: run
run:
	./out

clean:
	$(RM) $(OBJ)