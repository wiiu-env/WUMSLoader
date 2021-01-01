FROM wiiuenv/devkitppc:20210101

COPY --from=wiiuenv/wiiumodulesystem:20210101 /artifacts $DEVKITPRO

WORKDIR project