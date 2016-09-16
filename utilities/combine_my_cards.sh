#!/bin/bash

# Script to combine datacards of the same SUSY masses
# For each SUSY mass, makes two combined datacards - one with the ICHEP signal regions (baseline), and one with my corridor regions.
# Run this from inside your CMSSW/src/HiggsAnalysis/CombinedLimit/ directory.
# Right now, the script assumes the existence of two subdirectories: cards_uncombined and cards_combined.

reglist1='boost250,boost350,boost450,boost550,compr250,compr350,compr450,low250,low350,low450,high250,high350,high450,high550,high650'
reglist2='corridor250,corridor350,corridor450'

for susymass in `ls -1 cards_uncombined/ | grep -o 'T2tt_[[:digit:]]\+_[[:digit:]]\+' | sort | uniq`
do
	argstring_base=''
	argstring_corr=''
	for filename in `ls cards_uncombined/datacard_{compr*,boost*,low*,high*}_${susymass}.txt`;do argstring_base+=" ${filename}";done
	for filename in `ls cards_uncombined/datacard_corridor*_${susymass}.txt`;do argstring_corr+=" ${filename}";done
	echo Combining cards for ${susymass}...
	python scripts/combineCards.py ${argstring_base} > cards_combined/datacard_baseline_${susymass}.txt
	python scripts/combineCards.py ${argstring_corr} > cards_combined/datacard_corridor_${susymass}.txt
done