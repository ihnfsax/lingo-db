FROM ubuntu:jammy AS baseimg

# Basic
ARG http_proxy
ARG https_proxy
ENV http_proxy ${http_proxy}
ENV https_proxy ${https_proxy}
ENV HTTP_PROXY ${http_proxy}
ENV HTTPS_PROXY ${https_proxy}

ARG DEBIAN_FRONTEND=noninteractive
RUN if [ -n "${http_proxy}" ]; then echo "Acquire::http::Proxy \"${http_proxy}\";" > /etc/apt/apt.conf.d/proxy.conf; fi
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.ustc.edu.cn@g' /etc/apt/sources.list \
    && apt-get update && apt-get upgrade -y \
    && apt-get -y -V install python3 python3-dev python3-pip libtbb-dev ca-certificates \
    lsb-release zsh curl iputils-ping wget git cmake g++ ninja-build wget unzip \
    ccache curl build-essential gdb locales ssh vim tzdata manpages man-db man sudo
RUN wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb \
    && apt-get install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb \
    && apt-get update && apt-get install -y -V libarrow1200 libarrow-dev

RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="/root/.cargo/bin:$PATH"
RUN cargo install --force cbindgen

RUN git config --global user.name "ihnfsax" \
    && git config --global user.email "kylelynx7289@gmail.com" \
    && git config --global http.proxy ${http_proxy} \
    && git config --global https.proxy ${https_proxy}

RUN locale-gen zh_CN && locale-gen zh_CN.utf8
ENV LC_ALL zh_CN.UTF-8
ENV LANG zh_CN.UTF-8

ARG username=vscode
RUN useradd -ms /bin/zsh ${username} && usermod -aG sudo ${username} && mkdir -p /home/${username}/develop
USER ${username}
RUN cd ~ && git clone https://github.com/zsh-users/zsh-autosuggestions ~/.zsh/zsh-autosuggestions \
    && git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ~/.zsh/zsh-syntax-highlighting \
    && git clone https://github.com/ihnfsax/linux_settings_public.git \
    && cp linux_settings_public/tool_settings/zsh/.zshrc ~/.zshrc \
    && cp linux_settings_public/editors/vim_basic/.vimrc ~/.vimrc \
    && rm -rf linux_settings_public