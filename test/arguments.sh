#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/bin/arguments
$DIR/bin/arguments -t=1
$DIR/bin/arguments -t 2
$DIR/bin/arguments -t=3 -v
$DIR/bin/arguments -t 4 -f 49.0
$DIR/bin/arguments -t=5 -f=64.32
$DIR/bin/arguments --test=6 -float=11.0
$DIR/bin/arguments --test 7 --float 12.0 -s test7
$DIR/bin/arguments -t 8 -s "another test"
$DIR/bin/arguments -t 9 -s="test number nine" -f 45.0 -v
$DIR/bin/arguments -t 10 -v=32
$DIR/bin/arguments -t 11 "other arguments" 13 cde
$DIR/bin/arguments -t 12 -v -s --query
$DIR/bin/arguments -t 13 --float 12.0 "something else"
