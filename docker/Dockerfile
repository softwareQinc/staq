FROM ubuntu:latest
ENV TZ="America/Toronto"
RUN apt-get update && \
    apt-get -y update && \
    apt-get install -yq tzdata && \
    ln -fs /usr/share/zoneinfo/America/Toronto /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata

RUN apt-get install -y build-essential python3 python3-pip python3-dev \
    python3-venv cmake sudo git vim libgmp-dev

# Enable a normal user with sudo access
RUN useradd -m -c "softwareQ" sq
RUN echo '%sq ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Create a Python 3 virtual environment
RUN python3 -m venv venv

# Install Jupyter and other dependencies, followed by pystaq
COPY requirements.txt ./
RUN . venv/bin/activate && pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt && \
    pip install --no-cache-dir git+https://github.com/softwareqinc/staq

# Clone and install staq
WORKDIR /home/sq
RUN git clone --depth 1 --branch main https://github.com/softwareqinc/staq
WORKDIR /home/sq/staq
RUN cmake -B build -DINSTALL_SOURCES=ON && \
    cmake --build build --target all --parallel 4 && \
    sudo cmake --build build --target install
WORKDIR /home/sq

# Create a notebook directory for Jupyter
RUN mkdir notebooks
