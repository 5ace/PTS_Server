bin_PROGRAMS = extract match makeIndex joinIndices retrieve

extract_SOURCES = extract.cpp
extract_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/timer 
extract_LDADD = ../lib/libcdvs_main.la ../shared/libeval.la ../libraries/timer/libtimer.la -ljpeg -lrt

match_SOURCES = match.cpp
match_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/timer -I$(srcdir)/../libraries/map 
match_LDADD = ../lib/libcdvs_main.la ../shared/libeval.la ../libraries/map/libmap.la ../libraries/timer/libtimer.la -lrt

makeIndex_SOURCES = makeIndex.cpp
makeIndex_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src
makeIndex_LDADD = ../lib/libcdvs_main.la ../shared/libeval.la 

joinIndices_SOURCES = joinIndices.cpp
joinIndices_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src
joinIndices_LDADD = ../lib/libcdvs_main.la ../shared/libeval.la 

retrieve_SOURCES = retrieve.cpp
retrieve_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../shared -I$(srcdir)/../libraries/bitstream/src -I$(srcdir)/../libraries/timer -I$(srcdir)/../libraries/map 
retrieve_LDADD = ../lib/libcdvs_main.la ../shared/libeval.la ../libraries/map/libmap.la ../libraries/timer/libtimer.la -lrt

# the following libraries are needed if using libcdvs_bflog
# LDADD -lfftw3f -lfftw3f_threads  (where needed)
