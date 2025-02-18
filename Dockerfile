FROM ghcr.io/wiiu-env/devkitppc:20241128

COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:0.3.4-dev-20250218-b62548a /artifacts $DEVKITPRO

WORKDIR project
