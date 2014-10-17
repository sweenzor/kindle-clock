#!/bin/sh

eips -c
eips -c

eips 10 10 $(( (2849756400 - $(date +%s)) / (1) ))
