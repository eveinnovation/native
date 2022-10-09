get build image: ovidiudragoi/ffmpeg-build-ubuntu-20.04

ref: https://chowdera.com/2022/129/202205091526046812.html

For compilers to find ffmpeg@4 you may need to set:
export LDFLAGS="-L/opt/homebrew/opt/ffmpeg@4/lib"
export CPPFLAGS="-I/opt/homebrew/opt/ffmpeg@4/include"
