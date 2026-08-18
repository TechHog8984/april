#!/usr/bin/env bash

rm -rf skep/* && ../build/april --jar=$JAVA_HOME/jre/lib/rt.jar --output-folder=skep
