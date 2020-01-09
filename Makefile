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
