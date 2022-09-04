FROM wiiuenv/devkitppc:20220806

COPY --from=wiiuenv/wiiumodulesystem:20220904 /artifacts $DEVKITPRO

WORKDIR project