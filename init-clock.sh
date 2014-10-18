#!/bin/sh

# prevent kindle from sleeping
/etc/init.d/framework stop
/etc/init.d/powerd stop

# run
/mnt/us/display-clock.sh
