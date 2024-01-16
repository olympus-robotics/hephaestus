ARG BASE_IMAGE

# FROM arm64v8/ubuntu:22.04
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && \
    apt install -y --no-install-recommends x11-apps g++ mesa-utils make cmake-curses-gui ninja-build git gdb vim htop \
        curl zlib1g-dev libffi-dev libssl-dev libbz2-dev libsqlite3-dev iproute2 tk-dev texlive-latex-extra texlive-fonts-recommended dvipng cm-super libnotify-bin \
        pkg-config gpg wget ca-certificates git-lfs curl ccache ninja-build doxygen graphviz linux-generic python3-dev iproute2 python-is-python3 \
        net-tools iftop htop lsb-release software-properties-common gnupg bash-completion && \
        rm -rf /var/lib/apt/lists/*

RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt update && \
    rm /usr/share/keyrings/kitware-archive-keyring.gpg && \
    apt install -y kitware-archive-keyring cmake && \
    rm -rf /var/lib/apt/lists/*

ENV CLANG_VERSION=18
RUN wget https://apt.llvm.org/llvm.sh && \
    sed -i "s/add-apt-repository \"${REPO_NAME}\"/add-apt-repository \"${REPO_NAME}\" -y/g" llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh $CLANG_VERSION -y && \
    apt install -y clang-$CLANG_VERSION clang-tidy-$CLANG_VERSION clang-format-$CLANG_VERSION \
    llvm-$CLANG_VERSION-dev libc++-$CLANG_VERSION-dev libomp-$CLANG_VERSION-dev libc++abi-$CLANG_VERSION-dev libunwind-$CLANG_VERSION-dev && \
    rm -rf /var/lib/apt/lists/*

RUN  update-alternatives --install /usr/bin/clang clang /usr/bin/clang-$CLANG_VERSION $CLANG_VERSION \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-$CLANG_VERSION \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-$CLANG_VERSION \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-$CLANG_VERSION

RUN ln -s /usr/bin/clangd-${CLANG_VERSION} /usr/bin/clangd

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs > rust_installer.sh && chmod +x rust_installer.sh && ./rust_installer.sh -y
RUN . "$HOME/.cargo/env" && cargo install sccache --locked

RUN curl https://raw.githubusercontent.com/ninja-build/ninja/master/misc/bash-completion > $HOME/.ninja-autocomplete && echo "source $HOME/.ninja-autocomplete" >> $HOME/.bashrc
RUN sed -i "s/#force_color_prompt=yes/force_color_prompt=yes/g" $HOME/.bashrc && \
