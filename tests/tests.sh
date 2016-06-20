#!/bin/sh

if [ -x /usr/bin/tput ] && tput setaf > /dev/null; then
	color=yes
else
	color=
fi

if [ "$color" = yes ]; then
	RED="\033[31;1m"
	GREEN="\033[32;1m"
	BLUE="\033[34;1m"
	CLEAR="\033[0m"
fi

error=0

for path_file_to_test in ../samples/*; do
	file_to_test=$(basename $path_file_to_test)
	printf ${BLUE}" * testing ${file_to_test}\n"${CLEAR}

	if [ -f $file_to_test.output ]; then
		(../bob execute ../samples/$file_to_test) > /tmp/$file_to_test.interpreted 2> /dev/null
		printf "%-15.15s: compiler    " ${file_to_test}
		diff /tmp/$file_to_test.interpreted $file_to_test.output
		if [ "$?" -ne "0" ]; then
			printf ${RED}"failed\n"${CLEAR}
			error=1
		else
			printf ${GREEN}"succeed\n"${CLEAR}
		fi
	fi
	if [ -f $file_to_test.output ]; then
		(../bob interpret ../samples/$file_to_test) > /tmp/$file_to_test.interpreted 2> /dev/null
		printf "%-15.15s: interpreter " ${file_to_test}
		diff /tmp/$file_to_test.interpreted $file_to_test.output
		if [ "$?" -ne "0" ]; then
			printf ${RED}"failed\n"${CLEAR}
			error=1
		else
			printf ${GREEN}"succeed\n"${CLEAR}
		fi
	fi
	if [ -f $file_to_test.parsed ]; then
		../bob sem ../samples/$file_to_test > /tmp/$file_to_test.parsed 2> /dev/null
		printf "%-15.15s: parsing     " ${file_to_test}
		diff /tmp/$file_to_test.parsed $file_to_test.parsed
		if [ "$?" -ne "0" ]; then
			printf ${RED}"failed\n"${CLEAR}
			error=1
		else
			printf ${GREEN}"succeed\n"${CLEAR}
		fi
	fi
done

rm -f a.out
rm -f .generated.c

exit $error
