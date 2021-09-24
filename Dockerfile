FROM wiiuenv/devkitppc:20210920

COPY --from=wiiuenv/wiiumodulesystem:20210924 /artifacts $DEVKITPRO

WORKDIR project