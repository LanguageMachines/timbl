FROM alpine:latest
#VERSION can be:
# - stable: builds latest stable versions from source (default)
# - distro: uses packages as provided by Alpine Linux (may be slightly out of date)
# - devel: latest development version (git master/main branch)
ARG VERSION="stable"
LABEL org.opencontainers.image.authors="Maarten van Gompel <proycon@anaproy.nl>"
LABEL description="timbl - tilburg memory-based learner"

RUN mkdir -p /data
RUN mkdir -p /usr/src/timbl
COPY . /usr/src/timbl

RUN if [ "$VERSION" = "distro" ]; then \
        rm -Rf /usr/src/timbl &&\
        echo -e "----------------------------------------------------------\nNOTE: Installing latest release as provided by Alpine package manager.\nThis version may diverge from the one in the git master tree or even from the latest release on github!\nFor development, build with --build-arg VERSION=development.\n----------------------------------------------------------\n" &&\
        apk update && apk add timbl; \
    else \
        PACKAGES="libbz2 icu-libs libxml2 libgomp libstdc++" &&\
        BUILD_PACKAGES="build-base autoconf-archive autoconf automake libtool bzip2-dev icu-dev libxml2-dev git" &&\
        apk add $PACKAGES $BUILD_PACKAGES &&\ 
        cd /usr/src/ && ./timbl/build-deps.sh &&\
        cd timbl && sh ./bootstrap.sh && ./configure && make && make install &&\
        apk del $BUILD_PACKAGES && rm -Rf /usr/src; \
    fi

WORKDIR /

ENTRYPOINT [ "timbl" ]
