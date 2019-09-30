#!/bin/bash

set -e -x

if [ "$#" != "4" ]; then
   exit
fi

gcc cliente.c -o cliente

i=0

while [ $i -lt $3 ]; do
   ./cliente $1 $2 &
   sleep $4
   let i=i+1
done
