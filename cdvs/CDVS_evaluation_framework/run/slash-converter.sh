#! /bin/bash
#
# converts all backslash into slash in the distractor set files.
#
MYHOME=`pwd`
INPUTLIST="01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50"
DATADIR="/data/CDVS/Dataset-28072011"

# check data dir
if [ -d $DATADIR ]; then
   cd $DATADIR
else
   echo "$DATADIR does not exist" 
   exit 1
fi

for x in $INPUTLIST; do
    sed -i 's/\\/\//g' distracters_${x}.txt
done
