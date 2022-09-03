# socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\" && \
docker container rm $(docker container ls -aq) -f || echo "already closed"  && \
docker rmi -f $(docker images -aq) || echo "already closed" && \
docker-compose up