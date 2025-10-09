FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ARG NEW_USER=earthling
ARG UID=1000
ARG GID=1000

# Packages
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    sudo ca-certificates gnupg software-properties-common \
    build-essential gdb clang gcc pkg-config \
    autoconf automake libtool autotools-dev \
    git make valgrind \
    python3 python3-venv python3-pip \
    ripgrep less vim
RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/*

# Idempotent user/uid/gid setup
COPY .devcontainer/ensure-user.sh /usr/local/bin/ensure-user.sh
RUN chmod +x /usr/local/bin/ensure-user.sh
RUN /usr/local/bin/ensure-user.sh "${NEW_USER}" "${UID}" "${GID}"

# Passwordless sudo for the user
RUN echo "${NEW_USER} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/90-${NEW_USER}
RUN chmod 0440 /etc/sudoers.d/90-${NEW_USER}

USER ${NEW_USER}
WORKDIR /workspaces
SHELL ["/bin/bash", "-lc"]
