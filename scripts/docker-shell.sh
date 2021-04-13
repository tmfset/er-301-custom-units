#!/bin/bash
docker run --privileged -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units er-301-am335x-build-env:latest bash
