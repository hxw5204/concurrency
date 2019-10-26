## Edit this ##
SNUM=1
###############

HANDIN_FILE=handin-$(SNUM).tar.gz
TARGET = driver
OBJS += queue.o
OBJS += driver.o
OBJS += linked_list.o
OBJS += stress.o
OBJS += test.o
LIBS += -lpthread
#LIBS += -lpie

CC = clang
CFLAGS += -MMD -MP # dependency tracking flags
CFLAGS += -I./
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -Wconversion
CFLAGS += -fPIE
LDFLAGS += $(LIBS)

all: CFLAGS += -g -O2 # release flags
all: $(TARGET)

release: clean all

debug: CFLAGS += -g -O0 -D_GLIBC_DEBUG # debug flags
debug: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

DEPS = $(OBJS:%.o=%.d)
-include $(DEPS)

clean:
	-@rm $(TARGET) $(OBJS) $(DEPS) valgrind.log $(HANDIN_FILE) 2> /dev/null || true 

test: release
	@chmod +x grade.py
	python grade.py
bonus: release
	@chmod +x grade.py
	python grade.py bonus
handin: clean
	@tar cvzf $(HANDIN_FILE) *
	@echo "***"
	@echo "*** SUCCESS: Created $(HANDIN_FILE)"
	@echo "***"
