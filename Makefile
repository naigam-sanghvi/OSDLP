#
#  Open Space Data Link Protocol
#
#  Copyright (C) 2020 Libre Space Foundation (https://libre.space)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
 
LIBNAME    = libosdlp.a
QA_EXE     = test_osdlp

SRC_DIR    = src
QA_SRC_DIR = test
INCL_DIR   = include
DEPS       = ${INCL_DIR}/osdlp.h

SRC        = $(wildcard $(SRC_DIR)/*.c)
OBJ        = $(SRC:$(SRC_DIR)/%.c=$(SRC_DIR)/%.o)
QA_EXE_SRC = $(QA_SRC_DIR)/test.c
QA_SRC     = $(QA_SRC_DIR)/queue_util.c \
             $(QA_SRC_DIR)/test_pack_unpack.c \
             $(QA_SRC_DIR)/test_typeb_frames.c \
             $(QA_SRC_DIR)/test_normal_op.c \
             $(QA_SRC_DIR)/test_farm_window.c \
             $(QA_SRC_DIR)/test_tm.c

INCLUDES   += -I$(INCL_DIR)
QA_INC     = $(INCLUDES)
QA_INC     += -I$(QA_SRC_DIR)
CFLAGS     += -Wall -g
LDLIBS     += 
QA_LDLIBS  += -lcmocka -lpthread

all: $(QA_EXE)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INCL_DIR)/%.h
	$(CC) $(INCLUDES) $(CFLAGS) $(LDLIBS) -c -o $@ $<

$(LIBNAME): $(OBJ) $(DEPS)
	${AR} ru $@ $^

$(QA_EXE): $(LIBNAME) $(QA_SRC) $(QA_EXE_SRC)
	$(CC) $(QA_INC) $(LDFLAGS) $(CFLAGS) $^ $(QA_LDLIBS) ${LIBNAME} -o $@
	
.PHONY: test
test: $(QA_EXE)
	./$(QA_EXE)

coverage: $(QA_EXE)
	$(CC) -fprofile-arcs -ftest-coverage -g -fPIC -O0 $(QA_INC) $(INCLUDES) $(LDFLAGS) $(QA_SRC) $(QA_EXE_SRC) $(QA_LDLIBS) $(SRC) -o $(QA_EXE)
	./$(QA_EXE)
	gcovr --html --html-details -o coverage.html
	gcovr

clean:
	$(RM) $(OBJ)
	$(RM) $(QA_EXE)
	$(RM) $(LIBNAME)
	$(RM) *.gcda
	$(RM) *.gcno
	$(RM) *.gcov
	$(RM) *.html
