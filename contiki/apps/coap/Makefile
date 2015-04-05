DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = coap

TARGETDIRS += ../../platform

all: $(CONTIKI_PROJECT)

APPS += er-coap
APPS += rest-engine

ifdef ON
  CFLAGS += -DDEFAULT_LIGHT_ON=$(ON)
endif

ifdef FREQ
  CFLAGS += -DDEFAULT_LIGHT_FREQ=$(FREQ)
endif

ifdef DC
  CFLAGS += -DDEFAULT_LIGHT_DC=$(DC)
endif


ifdef BTLDR
  ifeq ($(BTLDR),0)
    CC2538_BSL_FORCE_CLOSE = 1
  endif
  CFLAGS += -DFLASH_CCA_CONF_BOOTLDR_BACKDOOR=$(BTLDR)
endif



CONTIKI = ../../../../contiki
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
