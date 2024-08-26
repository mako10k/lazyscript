ARG VARIANT="jammy"
FROM mcr.microsoft.com/vscode/devcontainers/base:1-${VARIANT}

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    build-essential \
    autoconf \
    automake \
    libtool \
    pkg-config \
    bison \
    flex \
    libgc-dev \
    clang \
    clang-tidy \
    clang-format \
    doxygen \
    graphviz \
    clangd \
    gdb \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*