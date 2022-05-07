FROM wiiuenv/devkitppc:20220507

COPY --from=wiiuenv/wiiumodulesystem:20220507 /artifacts $DEVKITPRO

WORKDIR project