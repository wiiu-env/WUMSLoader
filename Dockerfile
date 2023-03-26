FROM ghcr.io/wiiu-env/devkitppc:20230326

COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20230106 /artifacts $DEVKITPRO

WORKDIR project