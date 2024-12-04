ARG BASE_IMAGE

FROM ${BASE_IMAGE}

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends x11-apps g++ mesa-utils make cmake-curses-gui ninja-build git gdb vim htop \
    zlib1g-dev libffi-dev libssl-dev libbz2-dev libsqlite3-dev iproute2 tk-dev texlive-latex-extra texlive-fonts-recommended dvipng cm-super libnotify-bin \
    pkg-config gpg wget ca-certificates git-lfs ccache ninja-build doxygen graphviz linux-generic python3-dev python3-pip iproute2 python-is-python3 \
    net-tools iftop htop lsb-release software-properties-common gnupg bash-completion psmisc less tree apt-transport-https && \
    rm -rf /var/lib/apt/lists/*
RUN python3 -m pip --no-cache-dir install cmakelang==0.6.13

RUN wget -q -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    rm /usr/share/keyrings/kitware-archive-keyring.gpg && \
    apt-get install -y --no-install-recommends kitware-archive-keyring cmake && \
    rm -rf /var/lib/apt/lists/*

ENV CLANG_VERSION=19
RUN wget -q https://apt.llvm.org/llvm.sh && \
    sed -i "s/add-apt-repository \"${REPO_NAME}\"/add-apt-repository \"${REPO_NAME}\" -y/g" llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh $CLANG_VERSION -y && \
    apt-get install -y --no-install-recommends clang-$CLANG_VERSION clang-tidy-$CLANG_VERSION clang-format-$CLANG_VERSION \
    llvm-$CLANG_VERSION-dev libc++-$CLANG_VERSION-dev libomp-$CLANG_VERSION-dev libc++abi-$CLANG_VERSION-dev libunwind-$CLANG_VERSION-dev && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-$CLANG_VERSION $CLANG_VERSION \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-$CLANG_VERSION \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-$CLANG_VERSION \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-$CLANG_VERSION && \
    ln -s "/usr/bin/clangd-${CLANG_VERSION}" "/usr/bin/clangd"

ARG ZENOH_VERSION=1.0.0
RUN echo "deb [trusted=yes] https://download.eclipse.org/zenoh/debian-repo/ /" | tee /etc/apt/sources.list.d/zenoh.list >/dev/null && \
    ( apt-get update && apt-get install --no-install-recommends -y --allow-downgrades \
    zenohd=${ZENOH_VERSION} \
    zenoh-plugin-rest=${ZENOH_VERSION} \
    zenoh-bridge-ros2dds=${ZENOH_VERSION} \
    || true ) && \
    sed -i 's/systemctl /# remove if fixed by zenoh: systemctl /' /var/lib/dpkg/info/zenohd.postinst && \
    sed -i 's/systemctl /# remove if fixed by zenoh: systemctl /' /var/lib/dpkg/info/zenoh-bridge-ros2dds.postinst && \
    rm -rf /var/lib/apt/lists/*

ENV CARGO_HOME=/usr/local/cargo
ENV RUSTUP_HOME=/usr/local/rustup
RUN wget -q -O rust_installer.sh https://sh.rustup.rs && chmod +x rust_installer.sh && ./rust_installer.sh -y
RUN . "$CARGO_HOME/env" && cargo install sccache --locked && echo "export RUSTC_WRAPPER=sccache" >> "$HOME/.bashrc"
RUN chmod o+rw -R /usr/local/cargo/

RUN wget -q -O - https://pixi.sh/install.sh | PIXI_HOME=/usr/local bash

ARG BUF_VERSION="1.39.0"
RUN BIN="/usr/local/bin" && \
    wget -q -O "${BIN}/buf" "https://github.com/bufbuild/buf/releases/download/v${BUF_VERSION}/buf-$(uname -s)-$(uname -m)" &&\
    chmod +x "${BIN}/buf"

ARG MCAP_VERSION="0.0.49"
RUN BIN="/usr/local/bin" && \
    wget -q -O"${BIN}/mcap" "https://github.com/foxglove/mcap/releases/download/releases/mcap-cli/v${MCAP_VERSION}/mcap-linux-amd64" && \
    chmod +x "${BIN}/mcap"

COPY ./setup_bazel.sh /setup_bazel.sh
RUN /setup_bazel.sh
