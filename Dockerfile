# build stage
FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

WORKDIR /app

# install all dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git python3-pip libssl-dev nghttp2 \
    libopencv-dev libcurl4-openssl-dev tzdata \
    wget protobuf-compiler libprotobuf-dev && \
    ln -fs /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure --frontend noninteractive tzdata && \
    rm -rf /var/lib/apt/lists/*

# copy source code into the container
COPY . /app

# clean old build artifacts
RUN rm -rf /app/build /app/CMakeCache.txt /app/CMakeFiles || true

# regenerate protobuf sources inside the build stage so versions match
RUN protoc --cpp_out=./src ./telemetry.proto

# arena SDK extract / configuration (kept same as you had)
ARG ARENA_SDK_VERSION=0.1.78
ARG ARENA_SDK_ARCH=ARM64
ARG ARENA_SDK_FILE=ArenaSDK_v${ARENA_SDK_VERSION}_Linux_${ARENA_SDK_ARCH}.tar.gz
ARG ARENA_SDK_EXTRACTED_DIR=ArenaSDK_Linux_${ARENA_SDK_ARCH}

# Copy the Arena SDK based on build arg
COPY /external/${ARENA_SDK_FILE} /tmp/ArenaSDK.tar.gz

RUN tar -xzvf /tmp/ArenaSDK.tar.gz -C /tmp
RUN mv /tmp/${ARENA_SDK_EXTRACTED_DIR} /opt/arena_sdk

# configure library paths so the arena SDK libraries can be found
# Use find to automatically discover all Linux64* lib directories (works for ARM, x64, etc.)
RUN echo "/opt/arena_sdk/lib" > /etc/ld.so.conf.d/Arena_SDK.conf && \
    echo "/opt/arena_sdk/lib64" >> /etc/ld.so.conf.d/Arena_SDK.conf && \
    find /opt/arena_sdk/GenICam/library/lib -type d -name "Linux64*" \
    -exec bash -c 'echo "{}" >> /etc/ld.so.conf.d/Arena_SDK.conf' \;

# update dynamic linker cache so system can find our libraries
RUN ldconfig

# clean up the tarball to save space
RUN rm /tmp/ArenaSDK.tar.gz

# set up metavision SDK (bundled w/ arena SDK, need symlinks for version compatibility)
RUN if [ -d /opt/arena_sdk/Metavision/lib ]; then \
    echo "/opt/arena_sdk/Metavision/lib" > /etc/ld.so.conf.d/Metavision_SDK.conf && \
    cd /opt/arena_sdk/Metavision/lib && \
    ln -sf libmetavision_sdk_core.so.4.6.2 libmetavision_sdk_core.so.4 && \
    ln -sf libmetavision_sdk_base.so.4.6.2 libmetavision_sdk_base.so.4 && \
    ldconfig; \
    else echo "No Metavision SDK found, skipping"; fi

# initialize git submodules for external dependencies
RUN git submodule init && git submodule update

# configure & build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -- -j$(nproc)


# runtime stage (-> final image)

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

WORKDIR /app

# install only runtime dependencies into final image
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl-dev libopencv-dev libcurl4-openssl-dev nghttp2 tzdata tmux dos2unix \
    protobuf-compiler libprotobuf-dev && \
    ln -fs /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure --frontend noninteractive tzdata && \
    rm -rf /var/lib/apt/lists/*

# copy arena SDK and library configurations from build stage
COPY --from=build /opt/arena_sdk /opt/arena_sdk
COPY --from=build /etc/ld.so.conf.d/Arena_SDK.conf /etc/ld.so.conf.d/Arena_SDK.conf

# copy Metavision SDK config if it exists (conditional)
RUN if [ -d /opt/arena_sdk/Metavision/lib ]; then \
    echo "/opt/arena_sdk/Metavision/lib" > /etc/ld.so.conf.d/Metavision_SDK.conf; fi
RUN ldconfig

# copy compiled binaries from the build stage
COPY --from=build /app/build/camerafeed /app/

# copy runtime data files that the application needs
COPY --from=build /app/external /app/external
COPY --from=build /app/hotspots.csv /app/hotspots.csv
COPY --from=build /app/source.json /app/source.json
COPY --from=build /app/start_tmux.sh /app/start_tmux.sh
COPY --from=build /app/tag.txt /app/tag.txt

# set default command to run the camera application
CMD ["./camerafeed"]

# OpenCL is not mentioned in the dependencies but is used, 
# may need to install it if performance is critical