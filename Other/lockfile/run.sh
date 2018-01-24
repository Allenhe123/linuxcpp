#! /bin/bash
./trylock
echo "########after trylock, before append"
ls -i lock.txt
./append
echo "########after append"
ls -i lock.txt 
./trylock
echo "########after trylock, before rm_create"
ls -i lock.txt 
./rm_create
echo "########after rm_create, before trylock"
ls -i lock.txt 
./trylock