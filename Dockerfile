FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        cmake \
        g++ \
        make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=ON \
    && cmake --build /build -j"$(nproc)"

RUN ctest --test-dir /build --output-on-failure

FROM ubuntu:24.04

COPY --from=builder /build/incrify /usr/local/bin/incrify

ENTRYPOINT ["incrify"]
