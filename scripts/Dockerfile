FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    g++-16 \
    gcc-16 \
    cmake \
    make \
    git \
    && rm -rf /var/lib/apt/lists/*

ENV CXX=g++-16
ENV CC=gcc-16

WORKDIR /app
COPY . /app

RUN mkdir build && cd build && cmake .. && make -j$(nproc)

CMD ["cd", "build", "&&", "ctest", "--output-on-failure"]
