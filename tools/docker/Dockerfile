FROM ubuntu:jammy AS baseimg

ARG DEBIAN_FRONTEND=noninteractive
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.ustc.edu.cn@g' /etc/apt/sources.list \
    && apt-get update && apt-get upgrade -y \
    && apt-get -y -V install python3 python3-pip libtbb-dev ca-certificates lsb-release \
    wget git cmake g++ ninja-build wget unzip ccache curl build-essential gdb \
    locales ssh vim tzdata liblz4-dev libzstd-dev libre2-dev libprotobuf-dev
RUN wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb \
    && apt-get install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb \
    && apt-get update && apt-get install -y -V libarrow1200 libarrow-dev
RUN locale-gen zh_CN && locale-gen zh_CN.utf8
ENV LC_ALL zh_CN.UTF-8
ENV LANG zh_CN.UTF-8

RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="/root/.cargo/bin:$PATH"
RUN cargo install --force cbindgen

FROM baseimg AS lingodb
COPY . /repo
WORKDIR /repo
RUN git submodule init
RUN mkdir /build/lingodb
# RUN make build/lingodb-release/.buildstamp
# RUN cmake -G Ninja . -B /build/lingodb\
#     -DMLIR_DIR=/build/llvm/lib/cmake/mlir\
#     -DLLVM_EXTERNAL_LIT=/build/llvm/bin/llvm-lit\
#     -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release

RUN cmake --build /build/lingodb -j$(nproc)