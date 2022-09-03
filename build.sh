DIR=build && \
rm -rf ${DIR}; \
mkdir -p ${DIR}; \
cd ${DIR} && cmake .. && cmake --build . && bin/demo_sdl