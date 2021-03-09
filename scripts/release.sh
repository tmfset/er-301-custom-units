#!/bin/bash
docker run \
  -it \
  -v `pwd`:/er-301-custom-units \
  -w /er-301-custom-units \
  19090f80b2f5 \
  make clean all ARCH=am335x PROFILE=release
