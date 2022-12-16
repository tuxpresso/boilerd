FROM debian:bullseye-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        clang \
        git \
        cmake \
        make

WORKDIR /root
RUN git clone https://github.com/ahepp/pidc
RUN mkdir pidc/build && cd pidc/build && cmake .. && make && make install

FROM debian:bullseye-slim
RUN dpkg-reconfigure debconf -f noninteractive -p critical \
    && echo 'root:root' | chpasswd \
    && groupadd -g 1000 dev \
    && useradd -m -u 1000 -g dev dev

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        clang \
        clang-format \
        make

COPY --from=0 /usr/local/include/pidc.h /usr/include
COPY --from=0 /usr/local/lib/libpidc.a /usr/lib

USER dev
