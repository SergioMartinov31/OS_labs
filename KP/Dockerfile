FROM ubuntu:22.04

RUN apt update && apt install -y \
    gcc \
    make \
    strace

WORKDIR /lab
COPY . .

# Опционально: если есть скрипт сборки
RUN if [ -f "build.sh" ]; then chmod +x build.sh && ./build.sh; fi

CMD ["/bin/bash"]


# docker build -t strace-lab . - собрать образ
# docker run -it -v $(pwd):/lab strace-lab bash - запуск 