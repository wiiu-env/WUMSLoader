FROM wiiuenv/devkitppc:20210920

COPY --from=wiiuenv/wiiumodulesystem:20211031 /artifacts $DEVKITPRO

WORKDIR project