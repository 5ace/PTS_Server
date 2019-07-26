if WITH_BFLOG
  BFLOG_LA = libbflog.la
endif
noinst_LTLIBRARIES = libcdvs.la libeval.la $(BFLOG_LA)

# cdvs core library
libcdvs_la_SOURCES = ArithmeticCoding.cpp Database.cpp FeatureList.cpp Parameters.cpp Projective2D.cpp \
CsscCoordinateCoding.cpp Feature.cpp Points.cpp SCFVIndex.cpp SCFVData.cpp Buffer.cpp Buffer.h \
ArithmeticCoding.h Database.h Feature.h Parameters.h  CdvsDescriptor.h CdvsDescriptor.cpp CdvsPoint.h \
FeatureList.h Points.h CdvsException.h AlpOctave.h AlpOctave.cpp ImageBuffer.cpp ImageBuffer.h \
AbstractDetector.h AlpDetector.cpp AlpDetector.h AlpDetectorLowMem.cpp AlpDetectorLowMem.h \
CsscCoordinateCoding.h Match.h Projective2D.h SCFVIndex.h PointPairs.h PointPairs.cpp
libcdvs_la_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/vlfeat -I$(srcdir)/../libraries/Distrat -I$(srcdir)/../libraries/gmm-fisher -I$(srcdir)/../libraries/resampler  -I$(srcdir)/../libraries 

# evaluation framework
libeval_la_SOURCES = BoundingBox.h BoundingBox.cpp FileManager.cpp FileManager.h TraceManager.h TraceManager.cpp

# bflog has a dependency on libfftwf, so we keep it separate
if WITH_BFLOG
libbflog_la_SOURCES = AlpOctaveBF.h AlpOctaveBF.cpp AlpDetectorBF.cpp AlpDetectorBF.h
libbflog_la_CPPFLAGS = -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/vlfeat -I$(srcdir)/../libraries/fftw-3.3.3/api
endif

# Headers file that are going to be installed in <prefix>/include
include_HEADERS = CdvsPoint.h CdvsException.h PointPairs.h Parameters.h CdvsDescriptor.h Buffer.h ImageBuffer.h FeatureList.h Feature.h SCFVIndex.h AbstractDetector.h
