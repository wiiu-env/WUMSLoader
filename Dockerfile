FROM wiiuenv/devkitppc:20211229

COPY --from=wiiuenv/wiiumodulesystem:20211207 /artifacts $DEVKITPRO

WORKDIR project