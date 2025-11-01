# build stage
FROM ubuntu:22.04 AS build

# may need to change for the actual camera
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

WORKDIR /app

# install all dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential cmake git python3-pip libssl-dev nghttp2 \
      libopencv-dev libcurl4-openssl-dev tzdata libgeographic-dev \
      geographiclib-tools wget && \
    ln -fs /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure --frontend noninteractive tzdata && \
    rm -rf /var/lib/apt/lists/*

# install conan package manager for dependencies
RUN pip install --no-cache-dir conan

# copy source code into the container
COPY . /app

# clean any old build artificats
RUN rm -rf /app/build /app/CMakeCache.txt /app/CMakeFiles || true

# extract/install arena sdk in external in its tar.gz
# Build arguments for architecture-specific settings
ARG ARENA_SDK_VERSION=0.1.78
ARG ARENA_SDK_ARCH=ARM64
# Derived values - user shouldn't need to override these
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
    else \
      echo "No Metavision SDK found for this architecture, skipping"; \
    fi

# initialize git submodules for external dependencies
RUN git submodule init && git submodule update

# install c++ dependencies using Conan
RUN conan profile detect && \
    conan install . --output-folder=build --build=missing

# configure and build the project with CMake
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
RUN cmake --build build -- -j$(nproc)




# runtime stage (-> final image)

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

WORKDIR /app

# install only runtime dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      libssl-dev libopencv-dev libcurl4-openssl-dev nghttp2 tzdata && \
    ln -fs /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure --frontend noninteractive tzdata && \
    rm -rf /var/lib/apt/lists/*

# copy arena SDK and library configurations from build stage
COPY --from=build /opt/arena_sdk /opt/arena_sdk
COPY --from=build /etc/ld.so.conf.d/Arena_SDK.conf /etc/ld.so.conf.d/Arena_SDK.conf

# copy Metavision SDK config if it exists (conditional)
RUN if [ -f /opt/arena_sdk/Metavision/lib ]; then \
      echo "/opt/arena_sdk/Metavision/lib" > /etc/ld.so.conf.d/Metavision_SDK.conf; \
    fi

RUN ldconfig

# copy compiled binaries from the build stage
COPY --from=build /app/build/camerafeed /app/
COPY --from=build /app/build/curltest /app/

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