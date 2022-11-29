COROUTINE_SRC = $(wildcard common/*.cpp)
COROUTINE_SRC += $(wildcard coroutine/*.cpp)

LDFLAGS = -ldl -lpthread -lnsl -lm -lrt -lutil

OBJECT_PATH ?= build

TARGET = libcoroutine.a

OBJS := $(addsuffix .o,$(addprefix $(OBJECT_PATH)/,$(basename $(notdir $(COROUTINE_SRC)))))
DEPENDS := $(addsuffix .d,$(OBJS))

INCLUDE = -Icommon/ -Icoroutine/

RC = ar rc
CXX = g++
CXXFLAGS = -Wall $(INCLUDE) -g -D__linux__ -std=c++17 -m64

.PHONY : all clean

$(TARGET): $(OBJS)
	$(RC) $(TARGET) $(OBJS)

define make-cmd-cc
$2 : $1
	$$(info CXX $$<)
	$$(CXX) $$(CXXFLAGS) -MMD -MT $$@ -MF $$@.d -c -o $$@ $$<   
endef

$(foreach afile,$(COROUTINE_SRC),\
    $(eval $(call make-cmd-cc,$(afile),\
        $(addsuffix .o,$(addprefix $(OBJECT_PATH)/,$(basename $(notdir $(afile))))))))

clean:
	rm -rf libcoroutine.a;
	rm -rf $(OBJECT_PATH);
	mkdir $(OBJECT_PATH);

-include $(DEPENDS)