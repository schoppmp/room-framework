FROM gitlab.informatik.hu-berlin.de:4567/ti/software/bazel-builder:latest

ARG CACHE_USER
ARG CACHE_PASSWORD

WORKDIR ~/app
COPY . .

RUN /bin/env_wrapper bazel build //...