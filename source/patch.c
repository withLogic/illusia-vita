/*
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  patch.c
 * @brief Patching some of the .so internal functions or bridging them to native
 *        for better compatibility.
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include <sys/stat.h>
#include <stdio.h>
#include "utils/logger.h"
#include "audio.h"

extern so_module so_mod;
static so_hook CMvAppC1Ev_hook;
static so_hook CSaveMgrC1Ev_hook;

static so_hook _ZN10CShadowMgr4DrawEv_hook; // disable the shadows
static so_hook _ZN3CAI6UpdateEv_hook; // tick/tock ai -- i think this breaks the player ai?
static so_hook _ZN20GVUIPlayerController4DrawEv_hook; // dont draw the hud
static so_hook _ZN8CGsSound9PlaySoundEiih_hook; // audio bugs!
static so_hook _ZN8CGsSound4StopEv_hook;
static so_hook _ZN14CGsParticleMgr9UpdateAllEii_hook;

static so_hook setFrameSpeed_hook;

void *g_CMvApp_instance = NULL;
void *g_CSaveMgr_instance = NULL;

extern int settings_graphicsqualty;
extern int desiredFrameRate;
extern uint64_t targetUs;

char frame = 0;

void CMvAppC1Ev_patched(void *this) {
    g_CMvApp_instance = this;
    SO_CONTINUE(void *, CMvAppC1Ev_hook, this);
}

void CSaveMgrC1Ev_patched(void *this, int param) {
    g_CSaveMgr_instance = this;
    l_debug("CSaveMgrC1Ev::CSaveMgrC1Ev: Hooked into function");
    SO_CONTINUE(void *, CSaveMgrC1Ev_hook, this, param);
}

void _ZN10CShadowMgr4DrawEv_patched(void *this) {
    return;
}

void _ZN3CAI6UpdateEv_patched(void *this) {
    if(frame == 0){
        frame = 1;
        SO_CONTINUE(void *, _ZN3CAI6UpdateEv_hook, this);
    } else {
        frame = 0;
    }
}

void _ZN20GVUIPlayerController4DrawEv_patched(void *this) {
    return;
}

void _ZN8CGsSound9PlaySoundEiih_patched(void *this, int param2, int param3, int param4) {
    l_debug("_ZN8CGsSound9PlaySoundEiih_patched param2=%d, param3=%d, param4=%d", param2, param3, param4);
    audio_play_sound(param2, param3, param4);
}

void _ZN8CGsSound4StopEv_patched(void *this) {
    audio_stop_sound();
}

int _ZN14CGsParticleMgr9UpdateAllEii_patched(int param1, int param2, int param3) {
    l_debug("_ZN14CGsParticleMgr9UpdateAllEii_patched param1=%d, param2=%d, param3=%d", param1, param2, param3);
    return SO_CONTINUE(int, _ZN14CGsParticleMgr9UpdateAllEii_hook, param1, param2, param3);
}

int setFrameSpeed_patched(int param1) {
    l_debug("setFrameSpeed_patched param1=%d", param1);
    desiredFrameRate = param1;
    targetUs = 1000000ULL / desiredFrameRate;
}


void so_patch(void) {
    CMvAppC1Ev_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6CMvAppC1Ev"),
        (uintptr_t)&CMvAppC1Ev_patched);

    CSaveMgrC1Ev_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN8CSaveMgrC1Ev"),
        (uintptr_t)&CSaveMgrC1Ev_patched);

    /*
    _ZN3CAI6UpdateEv_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3CAI6UpdateEv"),
        (uintptr_t)&_ZN3CAI6UpdateEv_patched);
    */

    _ZN10CShadowMgr4DrawEv_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN10CShadowMgr4DrawEv"),
        (uintptr_t)&_ZN10CShadowMgr4DrawEv_patched);

    _ZN20GVUIPlayerController4DrawEv_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN20GVUIPlayerController4DrawEv"),
        (uintptr_t)&_ZN20GVUIPlayerController4DrawEv_patched);

    _ZN8CGsSound9PlaySoundEiih_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN8CGsSound9PlaySoundEiih"),
        (uintptr_t)&_ZN8CGsSound9PlaySoundEiih_patched);

    _ZN8CGsSound4StopEv_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN8CGsSound4StopEv"),
        (uintptr_t)&_ZN8CGsSound4StopEv_patched);

    _ZN14CGsParticleMgr9UpdateAllEii_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN14CGsParticleMgr9UpdateAllEii"),
        (uintptr_t)&_ZN14CGsParticleMgr9UpdateAllEii_patched);

    setFrameSpeed_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "setFrameSpeed"),
        (uintptr_t)&setFrameSpeed_patched);

}
