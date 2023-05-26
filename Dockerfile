FROM ubuntu:latest
ENV TZ="America/Toronto"
RUN apt-get update && \
    apt-get -y update && \
    apt-get install -yq tzdata && \
    ln -fs /usr/share/zoneinfo/America/Toronto /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata

RUN apt-get install -y \
    build-essential \
    python3.6 \
    python3-pip \
    python3-dev \
    git

RUN pip3 -q install pip --upgrade

RUN mkdir -p softwareq/staq/notebooks
WORKDIR softwareq/staq
COPY . .


RUN pip3 install jupyter matplotlib numpy
RUN pip3 install git+https://github.com/softwareqinc/staq

WORKDIR /src/notebooks

CMD ["jupyter", "notebook", "--port=8888", "--no-browser", "--ip=0.0.0.0", "--allow-root"] 
