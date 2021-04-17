FROM wiiuenv/devkitppc:20210414

COPY --from=wiiuenv/wiiumodulesystem:20210219 /artifacts $DEVKITPRO

WORKDIR project