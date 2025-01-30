FROM ghcr.io/wiiu-env/devkitppc:20241128

COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:0.3.3-dev-20250130-474ef70 /artifacts $DEVKITPRO

WORKDIR project
