FROM debian:bookworm-slim AS builder

WORKDIR /src/

RUN apt update && \
    apt install --no-install-recommends -yy \
        build-essential \
        libpng-dev \
        imagemagick \
        cmake -yy && \
    rm -rf /var/lib/apt/lists/*

COPY . /src/zero4tools

RUN echo "cf51006c86541b94e8305f456c4d47c5  /src/zero4tools/zero4/zero4.pce" | md5sum -c -

RUN cd /src/zero4tools/zero4 && \
    ./build.sh

FROM alpine AS release

VOLUME /output

COPY --from=builder /src/zero4tools/zero4/zero4_build.pce /

# Usage:  docker build . --target release --tag zero4tools
#         docker run --rm --volume ${PWD}:/output zero4tools
ENTRYPOINT cp /zero4_build.pce /output/zero4_build.pce