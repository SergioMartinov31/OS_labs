FROM ubuntu:22.04

RUN apt update && apt install -y \
    gcc \
    make \
    build-essential

WORKDIR /lab

COPY . .

RUN chmod +x build.sh && ./build.sh

CMD ["/bin/bash"]

# docker build -t oslab4 .  - у меня уже собран образ, поэтому не нужно
# docker run -it -v $(pwd):/lab oslab4 bash