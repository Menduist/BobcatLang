#!/bin/sh

for path_file_to_generate in ${1+"$@"}; do
	file_to_test=$(basename $path_file_to_generate)
	echo generating $file_to_test
	../bob interpret ../samples/$file_to_test > $file_to_test.output
	if [ "$?" -ne "0" ]; then
		echo $file_to_test interpreter failed /!\\
		rm $file_to_test.output
	fi

	../bob sem ../samples/$file_to_test > $file_to_test.parsed
	if [ "$?" -ne "0" ]; then
		echo $file_to_test parsing failed /!\\
		rm $file_to_test.parsed
	fi
	echo
done

exit $error
