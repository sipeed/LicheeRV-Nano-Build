#!/bin/bash

files=`find . -name "cv181x_*.dtsi"`

for i in $files
do
	echo $i
filename=${i#*cv181x_}
echo $filename
mv $i soph_$filename

done
