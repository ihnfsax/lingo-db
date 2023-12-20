# Example build command:
# docker build -f fedora-39-devcontainer.dockerfile --network=host \
#     --build-arg http_proxy=http://127.0.0.1:7890 \
#     --build-arg https_proxy=http://127.0.0.1:7890 \
#     --build-arg username=vscode \
#     -t fedora-39-devcontainer:latest .
# Available build arguments:
# `http_proxy`: proxy address for http
# `https_proxy`: proxy address for https
# `username`: username to create

FROM fedora:39

# Basic
ARG http_proxy
ARG https_proxy
ENV http_proxy ${http_proxy}
ENV https_proxy ${https_proxy}
ENV HTTP_PROXY ${http_proxy}
ENV HTTPS_PROXY ${https_proxy}

RUN dnf upgrade -y --refresh && dnf group install -y "Development Tools" \
    && dnf install -y vim git man-pages man-db zsh wget passwd iproute \
    python3-pip python glibc-locale-source glibc-langpack-en bzip2 procps

# timezone
ENV TZ=Asia/Chongqing
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# locale
ENV LANG=en_US.UTF-8
RUN localedef --force -i en_US -f UTF-8 en_US.UTF-8

RUN git config --global user.name "ihnfsax" \
    && git config --global user.email "kylelynx7289@gmail.com" \
    && git config --global http.proxy ${http_proxy} \
    && git config --global https.proxy ${https_proxy}

# C++
RUN dnf install -y g++ gdb lld cmake make llvm clang clang-tools-extra ninja-build \
    ccache automake libstdc++-static bison bison-devel flex libtool ncurses-devel \
    binutils-devel byacc perl


# Run this at last
# use vscode as regular user if argument `username` is not set
ARG username=vscode
RUN useradd -ms /bin/zsh ${username} && usermod -aG wheel ${username} && mkdir -p /home/${username}/develop
USER ${username}
RUN cd ~ && git clone https://github.com/zsh-users/zsh-autosuggestions ~/.zsh/zsh-autosuggestions \
    && git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ~/.zsh/zsh-syntax-highlighting \
    && git clone https://github.com/ihnfsax/linux_settings_public.git \
    && cp linux_settings_public/tool_settings/zsh/.zshrc ~/.zshrc \
    && cp linux_settings_public/editors/vim_basic/.vimrc ~/.vimrc \
    && rm -rf linux_settings_public

# Run the following commands in host machine to set password:
# set password for root:
#   docker exec -itu 0 {container_id} passwd
# set password for ${username}:
#   docker exec -itu 0 {container_id} passwd ${username}