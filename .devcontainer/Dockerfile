FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

LABEL description="This is a Docker Image for FZSD Framework build."

# 换源
COPY ubuntu.sources /etc/apt/sources.list.d/ubuntu.sources

# Install packages
RUN apt update && \
    apt install -y \
        build-essential git wget curl unzip xz-utils python3 \
        tar axel cmake ninja-build openocd ccache

# Set arm-none-eabi-gcc toolchain version
ARG TARGETPLATFORM
ARG BUILDPLATFORM
ARG TOOLS_PATH=/opt/gcc-arm-none-eabi
ARG ARM_VERSION=14.3.rel1

# Install ARM Toolchain
RUN if [ "$TARGETPLATFORM" = "linux/amd64" ]; then export ARM_ARCH=x86_64; \
    else export ARM_ARCH=aarch64; \
    fi \
    && echo "Downloading ARM GNU GCC for Platform: $ARM_ARCH" \
    && mkdir -p ${TOOLS_PATH} \
    && axel -n 64 -o gcc-arm-none-eabi.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_VERSION}/binrel/arm-gnu-toolchain-${ARM_VERSION}-${ARM_ARCH}-arm-none-eabi.tar.xz"

RUN tar xf gcc-arm-none-eabi.tar.xz --strip-components=1 -C ${TOOLS_PATH} --overwrite \
    && rm gcc-arm-none-eabi.tar.xz \
    && rm ${TOOLS_PATH}/*.txt \
    && rm -rf ${TOOLS_PATH}/share/doc

ENV TOOLCHAIN_EXPORT_STR="export PATH=\$PATH:${TOOLS_PATH}/bin"
RUN if [ `grep -c "$TOOLCHAIN_EXPORT_STR" "/etc/profile"` -ne '0' ]; then exit 0;\
    else echo "$TOOLCHAIN_EXPORT_STR" >> /etc/profile \
    && ln -s ${TOOLS_PATH}/bin/* /usr/local/bin; \
    fi \
    && arm-none-eabi-gcc --version

# setup python environment
RUN mv /usr/lib/python3.12/EXTERNALLY-MANAGED /usr/lib/python3.12/EXTERNALLY-MANAGED.bk

RUN apt install -y python3-pip

# Install python packages
RUN set -eux \
    && pip3 --version \
    && pip3 install -i https://mirrors.aliyun.com/pypi/simple/ kconfiglib \
    && exit 0

# Set locale environment
RUN apt install -y locales && locale-gen en_US.UTF-8

ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# Create User
WORKDIR /workspaces
ARG USERNAME=devuser
ARG GROUP_ID=1000
ARG USER_ID=1001

RUN groupadd -o -g ${GROUP_ID} -f ${USERNAME} \
    && useradd -u ${USER_ID} -g ${USERNAME} -G sudo,adm,users,root -m ${USERNAME} \
    && apt-get update \
    && apt-get install -y sudo \
    && echo ${USERNAME} ALL=\(ALL:ALL\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} \
    && echo %ubuntu ALL=\(ALL:ALL\) NOPASSWD:ALL > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN usermod -s /bin/bash ${USERNAME}

# Set the working directory
RUN chown -R ${USERNAME}:${USERNAME} /workspaces

RUN apt-get install -y graphviz doxygen

USER ${USERNAME}

# Set the default shell to bash
CMD ["/bin/bash"]
