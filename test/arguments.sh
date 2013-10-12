#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/arguments
$DIR/arguments -t=1
$DIR/arguments -t 2
$DIR/arguments -t=3 -v
$DIR/arguments -t 4 -f 49.0
$DIR/arguments -t=5 -f=64.32
$DIR/arguments --test=6 -float=11.0
$DIR/arguments --test 7 --float 12.0 -s test7
$DIR/arguments -t 8 -s "another test"
$DIR/arguments -t 9 -s="test number nine" -f 45.0 -v
$DIR/arguments -t 10 -v=32
$DIR/arguments -t 11 "other arguments" 13 cde
$DIR/arguments -t 12 -v -s --query
$DIR/arguments -t 13 --float 12.0 "something else"
