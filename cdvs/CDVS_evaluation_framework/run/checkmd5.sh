#! /bin/bash
#
# check md5 hash of descriptors
#
BINEXE="/usr/bin/md5sum"
EXPERIMENTS="graphics/original graphics/vga graphics/vga_jpeg paintings video buildings objects"
OUTFILE="all_md5sum.512.sm.txt"
MYHOME="/home/massimo/bin"
DATADIR="/home/massimo/data/CDVS/Dataset-20120210/" 

# check data dir
if [ -d $DATADIR ]; then
   cd $DATADIR
else
   echo "$DATADIR does not exist" 
   exit 1
fi

# check executable
if [ ! -x "$BINEXE" ]; then
   echo "$BINEXE is missing;"
   echo "please install $BINEXE before running this script."
   exit 4
fi

# check all files and directories
if [ ! -d "$MYHOME" ]; then
   echo "$MYHOME not found" 
   exit 2
fi

for x in $EXPERIMENTS; do
   if [ ! -d "$x" ]; then
      echo "$x not found" 
      exit 3
   fi
done

# reset output file
if [ -w "$OUTFILE" ]; then
   echo "removing old $OUTFILE" 
   rm $OUTFILE
fi

#run md5 checks
echo "computing ${OUTFILE}, please wait..."
for x in $EXPERIMENTS; do 
   echo "experiment: ${x}" 
   ALLFILES=`find ${x} -name "*.512.sm.cdvs" -print | sort`
   for k in ${ALLFILES}; do
   	${BINEXE} -b ${k} >> ${OUTFILE}
   done
done
echo "done."


