#!/usr/bin/env bash

SOURCE_LOC=$(readlink -f .)
DEST_LOC="/home/tknelms/led_strips/"

rsync -as --progress $SOURCE_LOC/* tknelms@pizero:${DEST_LOC}
