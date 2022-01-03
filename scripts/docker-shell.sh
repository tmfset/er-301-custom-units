#!/bin/bash
docker run \
  --privileged -it \
  --platform=linux/amd64 \
  -v `pwd`:/er-301-custom-units \
  -w /er-301-custom-units tomjfiset/er-301-am335x-build-env:1.1.2 \
  bash
