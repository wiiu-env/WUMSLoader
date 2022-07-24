#pragma once

extern "C" void __init();
extern "C" void init_wut();
extern "C" void fini_wut();

void doStart(int argc, char **argv);