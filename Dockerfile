# ryu_ldn_nx Development Container
# Based on devkitPro for Nintendo Switch development
#
# This image includes pre-built Atmosphere-libs (libstratosphere.a) and
# devkitPro packages so that CI workflows only need to compile the
# sysmodule and overlay — never the dependencies.
#
# Pre-built libs are installed into /opt/ryu_ldn_nx/ so that they
# won't be overwritten when a checkout is mounted at /workspace.

FROM devkitpro/devkita64:latest

LABEL maintainer="ryu_ldn_nx contributors"
LABEL description="Development environment for ryu_ldn_nx Switch sysmodule with pre-built dependencies"

# Set environment variables
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=${DEVKITPRO}/devkitARM
ENV DEVKITA64=${DEVKITPRO}/devkitA64
ENV PATH=${DEVKITPRO}/tools/bin:${DEVKITA64}/bin:${PATH}

# Install additional system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    iputils-ping \
    git \
    curl \
    wget \
    zip \
    unzip \
    python3 \
    python3-pip \
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --break-system-packages gcovr

# Install Switch development libraries via dkp-pacman
RUN dkp-pacman -Syyu --noconfirm && \
    dkp-pacman -S --noconfirm \
    switch-dev \
    libnx \
    switch-curl \
    switch-zlib \
    switch-mbedtls \
    switch-miniupnpc \
    devkitA64-gdb

# Configure GDB to allow auto-loading scripts from any path
RUN mkdir -p /root/.config/gdb && \
    echo "set auto-load safe-path /" > /root/.config/gdb/gdbinit

# Create non-root user (UID/GID will be updated by VSCode's updateRemoteUserUID)
RUN useradd -m -s /bin/bash devuser

# Allow git to work with mounted volumes (different ownership inside Docker)
RUN git config --global --add safe.directory '*'

# ---------------------------------------------------------------------------
# Pre-build Atmosphere-libs (libstratosphere.a)
# The submodule sources are COPY'd in from the host checkout so that the
# Docker image contains the compiled library. CI workflows that use this
# image never need to rebuild it.
#
# Build happens in /tmp/build to keep /workspace clean for volume mounts.
# After building, the compiled library is installed to /opt/ryu_ldn_nx/
# where the sysmodule Makefile can find it, and the build artifacts are
# cleaned up.
# ---------------------------------------------------------------------------
COPY sysmodule/Atmosphere-libs/ /tmp/Atmosphere-libs-build/

RUN cd /tmp/Atmosphere-libs-build/libstratosphere && \
    make -j$(nproc) nx_release && \
    echo "libstratosphere.a built successfully"

# Install the compiled library and headers to /opt/ryu_ldn_nx/
# This path is separate from /workspace so it won't be overwritten by
# volume mounts or GitHub Actions checkouts.
RUN mkdir -p /opt/ryu_ldn_nx/libstratosphere/lib/nintendo_nx_arm64_armv8a/release && \
    mkdir -p /opt/ryu_ldn_nx/libstratosphere/include && \
    mkdir -p /opt/ryu_ldn_nx/libvapours/include && \
    mkdir -p /opt/ryu_ldn_nx/config/templates && \
    mkdir -p /opt/ryu_ldn_nx/libstratosphere && \
    cp /tmp/Atmosphere-libs-build/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a \
       /opt/ryu_ldn_nx/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/ && \
    cp -r /tmp/Atmosphere-libs-build/libstratosphere/include/* \
          /opt/ryu_ldn_nx/libstratosphere/include/ && \
    cp /tmp/Atmosphere-libs-build/libstratosphere/stratosphere.specs \
       /opt/ryu_ldn_nx/libstratosphere/ 2>/dev/null || true && \
    cp /tmp/Atmosphere-libs-build/libstratosphere/discard-ehframe.ld \
       /opt/ryu_ldn_nx/libstratosphere/ 2>/dev/null || true && \
    cp -r /tmp/Atmosphere-libs-build/libvapours/include/* \
          /opt/ryu_ldn_nx/libvapours/include/ && \
    cp -r /tmp/Atmosphere-libs-build/config/templates/* \
          /opt/ryu_ldn_nx/config/templates/ && \
    echo "Installed pre-built libs to /opt/ryu_ldn_nx/" && \
    ls -la /opt/ryu_ldn_nx/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a

# Clean up build directory
RUN rm -rf /tmp/Atmosphere-libs-build

# Create workspace directory (will be mounted over at runtime)
WORKDIR /workspace

# Set default shell to bash
SHELL ["/bin/bash", "-c"]

# Default command
CMD ["/bin/bash"]