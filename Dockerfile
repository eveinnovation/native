FROM  ubuntu:20.04

WORKDIR /data

RUN     apt-get -yqq update && \
        apt-get install -yq --no-install-recommends ca-certificates expat libgomp1 && \
        apt-get autoremove -y && \
        apt-get clean -y

ARG DEBIAN_FRONTEND=noninteractive

RUN      buildDeps="autoconf \
                    automake \
                    libnvidia-gl-460 \
                    cmake \
                    curl \
                    bzip2 \
                    libexpat1-dev \
                    g++ \
                    gcc \
                    git \
                    gperf \
                    libtool \
                    make \
                    meson \
                    nasm \
                    perl \
                    pkg-config \
                    python \
                    libsdl2-dev \
                    libssl-dev \
                    # libnvidia-gl-440 \
                     automake \
        cmake \
        curl \
        bzip2 \
        libexpat1-dev \
        g++ \
        gcc \
        git \
        gperf \
        build-essential \
        mesa-utils \
        libgl1-mesa-glx \
        x11-apps \
        libtool \
        make \
        pkg-config \
        python \
        libssl-dev \
        libsdl2-dev \
        libsdl2-image-dev \
        libnuma1 \
        libnuma-dev \
        libpulse-dev \
        libgstrtspserver-1.0-dev \
        gstreamer1.0-rtsp \
        libgtk-3-dev \
        libnm-dev \
        libpulse-mainloop-glib0 \
        build-essential \
        libgtk-3-dev \
                    yasm \
                    libnuma1 \
                    libnuma-dev \
                    zlib1g-dev" && \
        apt-get -yqq update && \
        apt-get install -yq --no-install-recommends ${buildDeps}
        
ENV DISPLAY=:0
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/ffmpeg/lib/:/data/ffmpeg/lib64/