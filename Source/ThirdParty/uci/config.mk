ifeq ($(ARCH),ppc)
COMMON_CC_FLAGS 		+=-DAPX_BIG_ENDIAN
CROSS_COMPILE = ppc_82xx-
endif

ifeq ($(ARCH),i386)
CROSS_COMPILE = i386-linux-
endif

CC				= $(CROSS_COMPILE)gcc
AS				= $(CROSS_COMPILE)as
AR				= $(CROSS_COMPILE)ar
LD				= $(CROSS_COMPILE)gcc
RES				= $(CROSS_COMPILE)fares
STRIP			= $(CROSS_COMPILE)strip
CP				= cp
MV				= mv

#COMMON_CC_FLAGS    += -Wimplicit-function-declaration
COMMON_CC_FLAGS    += -c -g
ADD_INCLUDE 		+= -I$(BUILD_BASE)/uci -I$(BUILD_BASE)/include
LDFLAGS 			+= -L$(BUILD_BASE)/uci
#LDFLAGS 			+= -static	-L$(BUILD_BASE)/uci

OBJS                    = 
TARGET                  = 
LDFLAGS                 += 
DEPENDS                 +=


ifeq ($(DLL),yes)
LDFLAGS    		+= -shared
COMMON_CC_FLAGS	+= -fPIC
endif


#
#µ÷ÊÔ´¦Àí
#
ifeq ($(DEBUG),yes)
COMMON_CC_FLAGS 		+=-D__DEBUG__=1 -g
endif

