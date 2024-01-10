FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
        build-essential \
        gcc-multilib \
    && rm -rf /var/lib/apt/lists/*
