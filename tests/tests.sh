#!/bin/sh

error=0

for path_file_to_test in ../samples/*; do
	file_to_test=$(basename $path_file_to_test)
	echo testing $file_to_test
	if [ -f $file_to_test.interpreted ]; then
		../timinterpreter ../samples/$file_to_test > /tmp/$file_to_test.interpreted
		diff /tmp/$file_to_test.interpreted $file_to_test.interpreted
		if [ "$?" -ne "0" ]; then
			echo $file_to_test interpreter failed /!\\
			error=1
		else
			echo $file_to_test interpreter succeed
		fi
	fi
	if [ -f $file_to_test.parsed ]; then
		../timparser ../samples/$file_to_test > /tmp/$file_to_test.parsed
		diff /tmp/$file_to_test.parsed $file_to_test.parsed
		if [ "$?" -ne "0" ]; then
			echo $file_to_test parsing failed /!\\
			error=1
		else
			echo $file_to_test parsing succeed
		fi
	fi
	echo
done

exit $error
