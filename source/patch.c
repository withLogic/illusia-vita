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
#include <stdlib.h>
#include <string.h>
#include "utils/logger.h"
#include "audio.h"

extern so_module so_mod;
extern int desiredFrameRate;
extern uint64_t targetUs;
extern currentSkillBank;

extern void (*CGsGraphics_DrawRect)(void *this, int x, int y, int w, int h, unsigned int color);

static so_hook CMvAppC1Ev_hook;
static so_hook setFrameSpeed_hook;
static so_hook CMvLayerData_PreLoad_hook;
static so_hook _ZN20GVUIPlayerController4DrawEv_hook;
static so_hook _ZN8CGsSound9PlaySoundEiih_hook;
static so_hook _ZN14CGsEncryptFile9LoadBeginEPKcb_hook;
static so_hook _ZN9CMvGameUI13DrawQuickSlotEv_hook;
static so_hook _ZN11CGsGraphics10InitializeEbbbi_hook;

void *g_CMvApp_instance = NULL;
void *g_CGsSound_instance = NULL;
void *g_CGsGraphics_instance = NULL;

static void *(*MC_knlCalloc_ptr)(size_t) = NULL;

void CMvAppC1Ev_patched(void *this) {
    g_CMvApp_instance = this;
    SO_CONTINUE(void *, CMvAppC1Ev_hook, this);
}

void _ZN11CGsGraphics10InitializeEbbbi_patched(void *this, int param2, int param3, int param4, int param5) {
    g_CGsGraphics_instance = this;
    l_debug("[_ZN11CGsGraphics10InitializeEbbbi_patched]");
    SO_CONTINUE(void *, _ZN11CGsGraphics10InitializeEbbbi_hook, this, param2, param3, param4, param5);
}

int setFrameSpeed_patched(int param1) {
    l_debug("setFrameSpeed_patched param1=%d", param1);
    desiredFrameRate = param1 * 2;

    targetUs = 1000000ULL / desiredFrameRate;
}   

/*
AI was used to help solve this bug, evidently there is a bug in the orignal game 
where the game does an invalid null check. On an Android device this ends up working 
fine, but with the Vita and SO_LOADER it cause a crash.
*/
int CMvLayerData_PreLoad_patched(int *a1, int a2, int a3, int a4, unsigned int a5) {
    int *v5 = a1;
    int v6 = a3;
    int v7 = a4;
    int v8 = 0;

    if (a4 != 0) {
        memcpy(a1 + 1, (const void *)(a5 + a4), 8);

        void *v9 = MC_knlCalloc_ptr(2 * v6);
        v5[3] = (int)v9;
        memcpy(v9, (const void *)(a5 + 8 + v7), 2 * v6);

        int v10 = v5[1];
        v8 = a5 + 8 + 2 * v6;
        size_t v11 = 19 * v10;
        size_t v12;

        if (v11) {
            void *v15 = MC_knlCalloc_ptr(v11);
            v5[4] = (int)v15;
            memcpy(v15, (const void *)(v8 + v7), v11);
            v8 += v11;
            v12 = 20 * v5[2];
            if (!v12) return v8;
        } else {
            v12 = 20 * v5[2];
            if (!v12) return v8;
        }

        void *v14 = MC_knlCalloc_ptr(v12);
        v5[5] = (int)v14;
        memcpy(v14, (const void *)(v8 + v7), v12);
        v8 += v12;
    }

    return v8;
}

void _ZN20GVUIPlayerController4DrawEv_patched(void *this) {
    // stubbing this out hides the on screen dpad and buttons
}

int _ZN8CGsSound9PlaySoundEiih_patched(void *this, int param1, int param2, int param3) {
    l_debug("[_ZN8CGsSound9PlaySoundEiih_patched] param1=%d, parm2=%d, param3=%d", param1, param2, param3);
    if(!g_CGsSound_instance){
        g_CGsSound_instance = this;
    }

    return SO_CONTINUE(int, _ZN8CGsSound9PlaySoundEiih_hook, this, param1, param2, param3);
}

int _ZN14CGsEncryptFile9LoadBeginEPKcb_patched(void *this, const char *filename, int param3){
    l_debug("[_ZN14CGsEncryptFile9LoadBeginEPKcb_patched] attempting to load file, %s with param3=%d", filename, param3);
    return SO_CONTINUE(int, _ZN14CGsEncryptFile9LoadBeginEPKcb_hook, this, filename, param3);
}

int32_t CGsSound_GetSndIDFromListIdx(void *this, int32_t idx) {
    int32_t count = *(int32_t *)((uint8_t *)this + 116);

    if (idx < 0 || idx >= count) {
        return -1;
    }
    int32_t *list = *(int32_t **)((uint8_t *)this + 112);

    return list[idx];
}

void DrawActiveBankHighlight(int g_activeBank) {
    static const int leftSlots[4]  = {0, 1, 4, 5};
    static const int rightSlots[4] = {2, 3, 7, 6};
    const int *slots = g_activeBank ? rightSlots : leftSlots;

    static const struct { int x, y, w, h; } slotRects[8] = {
        {7,   58, 18, 18}, // slot 0
        {56,  58, 18, 18}, // slot 1
        {406, 58, 18, 18}, // slot 2
        {455, 58, 18, 18}, // slot 3
        {7,  120, 18, 18}, // slot 4
        {56, 120, 18, 18}, // slot 5
        {406,120, 18, 18}, // slot 6
        {455,120, 18, 18}, // slot 7
    };

    for (int i = 0; i < 4; i++) {
        int s = slots[i];
        if(g_CGsGraphics_instance){
            CGsGraphics_DrawRect(g_CGsGraphics_instance,
                slotRects[s].x, slotRects[s].y, slotRects[s].w, slotRects[s].h,
                0xFFFFFFFF);
        }
    }
}

void _ZN9CMvGameUI13DrawQuickSlotEv_patched(void *this) {
    SO_CONTINUE(void *, _ZN9CMvGameUI13DrawQuickSlotEv_hook, this);
    DrawActiveBankHighlight(currentSkillBank);
}


void so_patch(void) {
    MC_knlCalloc_ptr = (void *(*)(size_t))so_symbol(&so_mod, "MC_knlCalloc");

    CMvAppC1Ev_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6CMvAppC1Ev"),
        (uintptr_t)&CMvAppC1Ev_patched);

    CMvLayerData_PreLoad_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN12CMvLayerData7PreLoadE15EnumObjectLayerilj"),
        (uintptr_t)&CMvLayerData_PreLoad_patched);

    setFrameSpeed_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "setFrameSpeed"),
        (uintptr_t)&setFrameSpeed_patched);

    _ZN20GVUIPlayerController4DrawEv_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN20GVUIPlayerController4DrawEv"),
        (uintptr_t)&_ZN20GVUIPlayerController4DrawEv_patched);

    _ZN8CGsSound9PlaySoundEiih_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN8CGsSound9PlaySoundEiih"),
        (uintptr_t)&_ZN8CGsSound9PlaySoundEiih_patched);

    
    _ZN14CGsEncryptFile9LoadBeginEPKcb_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN14CGsEncryptFile9LoadBeginEPKcb"),
        (uintptr_t)&_ZN14CGsEncryptFile9LoadBeginEPKcb_patched);


    _ZN11CGsGraphics10InitializeEbbbi_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN11CGsGraphics10InitializeEbbbi"),
        (uintptr_t)&_ZN11CGsGraphics10InitializeEbbbi_patched);

    _ZN9CMvGameUI13DrawQuickSlotEv_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN9CMvGameUI13DrawQuickSlotEv"),
        (uintptr_t)&_ZN9CMvGameUI13DrawQuickSlotEv_patched);
}