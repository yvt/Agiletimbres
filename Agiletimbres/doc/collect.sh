#!/bin/sh

#  Script.sh
#  XTBook
#
#  Created by Kawada Tomoaki on 5/20/11.
#  Copyright 2011 Nexhawks. All rights reserved.

SRCPATH="."

SRCFILES="$SRCPATH/*.cpp $SRCPATH/tcw/*/*.cpp $SRCPATH/TPList/*.cpp"
SRCFILES+=" $SRCPATH/*.h $SRCPATH/TPList/*.h"
SRCFILES+=" $SRCPATH/tcw/*/*/*.h"

cat $SRCFILES
