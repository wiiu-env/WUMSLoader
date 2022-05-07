#pragma once

extern "C" void __init();
extern "C" void __init_wut();
extern "C" void __fini_wut();

void doStart(int argc, char **argv);