#!/bin/bash

echo "Start $1 client"

for i in `seq 0 $1`
do
	./epoll_cli  > /dev/null &
	sleep 1;
done
