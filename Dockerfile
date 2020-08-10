FROM wiiuenv/devkitppc:20200810

COPY --from=wiiuenv/wiiumodulesystem:20200626 /artifacts $DEVKITPRO

WORKDIR project