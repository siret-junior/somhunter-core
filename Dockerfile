FROM ubuntu:20.04

ENV TZ=Europe/Prague
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get -q update > /dev/null
RUN apt-get install -yq --no-install-recommends \
    build-essential dos2unix python3 python3-pip python-is-python3 \
    libcurl4-openssl-dev libopencv-dev libcpprest-dev > /dev/null

# Install the latest CMake
#   - https://blog.kitware.com/cmake-python-wheels/
RUN pip3 install -q --upgrade cmake urllib3 > /dev/null

EXPOSE 8080
