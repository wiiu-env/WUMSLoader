FROM ghcr.io/wiiu-env/devkitppc:20241128

COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20250208 /artifacts $DEVKITPRO

WORKDIR project
