#!/bin/sh

mv $1 $1.bak
cat BOILERPLATE $1.bak > $1
