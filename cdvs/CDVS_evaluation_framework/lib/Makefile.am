if WITH_BFLOG
  BFLOG_LA = libcdvs_bflog.la
endif
if WITH_LOWMEM
  LOWMEM_LA = libcdvs_lowmem.la
endif
lib_LTLIBRARIES = libcdvs_main.la $(BFLOG_LA) $(LOWMEM_LA)

#definitions for the CDVS library (main version)
libcdvs_main_la_SOURCES = CdvsInterface.h CdvsInterface.cpp CdvsConfigurationImpl.h CdvsConfigurationImpl.cpp CdvsClientImpl.h CdvsClientImpl.cpp CdvsServerImpl.h CdvsServerImpl.cpp
libcdvs_main_la_CPPFLAGS = -DMAIN -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/Distrat
libcdvs_main_la_LIBADD = ../shared/libcdvs.la ../libraries/Distrat/libdistrat.la ../libraries/bitstream/src/libbitstream.la ../libraries/vlfeat/vl/libvlfeat.la ../libraries/resampler/libresampler.la ../libraries/gmm-fisher/libfisher.la

#definitions for the CDVS library (low memory variant)
if WITH_LOWMEM
libcdvs_lowmem_la_SOURCES = CdvsInterface.h CdvsInterface.cpp CdvsConfigurationImpl.h CdvsConfigurationImpl.cpp CdvsClientLowMem.h CdvsClientLowMem.cpp CdvsClientImpl.cpp CdvsServerImpl.h CdvsServerImpl.h CdvsServerImpl.cpp
libcdvs_lowmem_la_CPPFLAGS = -DLOWMEM -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/Distrat
libcdvs_lowmem_la_LIBADD = ../shared/libcdvs.la ../libraries/Distrat/libdistrat.la ../libraries/bitstream/src/libbitstream.la ../libraries/vlfeat/vl/libvlfeat.la ../libraries/resampler/libresampler.la ../libraries/gmm-fisher/libfisher.la
endif

#definitions for the CDVS library (bflog variant)
if WITH_BFLOG
libcdvs_bflog_la_SOURCES = CdvsInterface.h CdvsInterface.cpp CdvsConfigurationImpl.h CdvsConfigurationImpl.cpp CdvsClientBflog.h CdvsClientBflog.cpp CdvsClientImpl.cpp CdvsServerImpl.h CdvsServerImpl.h CdvsServerImpl.cpp
libcdvs_bflog_la_CPPFLAGS = -DBFLOG -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/Distrat
libcdvs_bflog_la_LIBADD = ../shared/libbflog.la ../shared/libcdvs.la ../libraries/Distrat/libdistrat.la ../libraries/bitstream/src/libbitstream.la ../libraries/vlfeat/vl/libvlfeat.la ../libraries/resampler/libresampler.la ../libraries/gmm-fisher/libfisher.la
endif


# Headers file that are going to be installed in <prefix>/include
include_HEADERS = CdvsInterface.h

