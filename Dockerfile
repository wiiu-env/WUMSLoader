FROM wiiuenv/devkitppc:20200810

COPY --from=wiiuenv/wiiumodulesystem:20200812 /artifacts $DEVKITPRO

WORKDIR project