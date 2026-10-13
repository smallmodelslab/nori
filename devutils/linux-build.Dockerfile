# Helium's Linux build image, plus quilt for nori's patch series and venv for
# its tools. devutils/build-linux.sh builds Helium's image first, as
# chromium-builder:trixie-slim.
FROM chromium-builder:trixie-slim
USER root
RUN apt-get -y update && apt-get -y install quilt python3-venv && rm -rf /var/lib/apt/lists/*
USER builder
