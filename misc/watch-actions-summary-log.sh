#!/bin/sh

DIR="../logs/user-actions-summary/admin/"
tail -n0 -f $DIR$(ls -t $DIR | head -n1)

read
