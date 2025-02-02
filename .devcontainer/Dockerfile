ARG CUDA_VERSION=11.8.0
ARG OS_VERSION=ubuntu22.04

FROM nvcr.io/nvidia/cuda:${CUDA_VERSION}-devel-${OS_VERSION}

LABEL maintainer="Bernd Doser <bernd.doser@h-its.org>"

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        apt-transport-https \
        build-essential \
        ca-certificates \
        curl \
        dirmngr \
        ffmpeg \
        git \
        libsm6 \
        libxext6 \
        python3 \
        python3-dev \
        python3-pip \
        software-properties-common \
        sudo \
        vim \
        wget \
        moreutils \
        python3-tk \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

RUN pip install --upgrade pip \
 && hash -r pip3 \
 && pip install \
        cmake \
        conan \
        matplotlib \
        numpy

ARG USERNAME=vscode
ARG USER_UID=15005
ARG USER_GID=15000

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    #
    # [Optional] Add sudo support. Omit if you don't need to install software after connecting.
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# ********************************************************
# * Anything else you want to do like clean up goes here *
# ********************************************************

# [Optional] Set the default user. Omit if you want to keep the default as root.
USER $USERNAME
