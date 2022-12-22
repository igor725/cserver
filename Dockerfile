# syntax=docker/dockerfile:1
FROM ubuntu:latest
WORKDIR /cserver
ENTRYPOINT ./out/$(gcc -dumpmachine)-dbg/server

COPY . .

RUN apt-get update && apt-get install -y \
	gcc make zlib1g-dev git-core libluajit-5.1-dev pkg-config dpkg-dev

RUN git pull; exit 0
RUN git clone https://github.com/igor725/cs-base ../cs-base
RUN git clone https://github.com/igor725/cs-lua ../cs-lua

RUN ./build wall dbg
RUN ./build wall dbg pb base install
RUN ./build wall dbg pb lua install pkg

EXPOSE 25565/tcp
