FROM python:3.11
WORKDIR /Video_analyzer

# Install system dependencies for OpenCV and MoviePy
RUN apt-get update && apt-get install -y \
    fonts-dejavu-core \
    fontconfig \
    libx11-6 \
    libxext6 \
    libxrender1 \
    libsm6 \
    libice6 \
    git \
    curl \
    build-essential \
    libgl1 \
    libglib2.0-0 \
    bash \
    ffmpeg \
 && rm -rf /var/lib/apt/lists/*

# Copy everything into container
COPY . .

# Install Python packages inside the container
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt
