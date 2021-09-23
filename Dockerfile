FROM wiiuenv/devkitppc:20210920

COPY --from=wiiuenv/wiiumodulesystem:20210917 /artifacts $DEVKITPRO

WORKDIR project