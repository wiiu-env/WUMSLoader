FROM wiiuenv/devkitppc:20211106

COPY --from=wiiuenv/wiiumodulesystem:20211207 /artifacts $DEVKITPRO

WORKDIR project