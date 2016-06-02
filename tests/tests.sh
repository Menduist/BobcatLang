#!/bin/sh

error=0

totest="if.tim helloworld.tim comments.tim"

for file_to_test in $totest; do
	echo testing $file_to_test
	../tim ../samples/$file_to_test > /tmp/$file_to_test.output
	diff /tmp/$file_to_test.output $file_to_test.output
	if [ "$?" -ne "0" ]; then
		echo $file_to_test failed /!\\
		error=1
	else
		echo $file_to_test succeed
	fi
	echo
done

exit $error
