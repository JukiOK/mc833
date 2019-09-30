#!/bin/bash

set -e -x

kill -9 `ps aux | grep cliente | grep -v grep | awk '{print $2}'` 
