#include <stdio.h>
#include "utils/init.h"
#include "utils/glutil.h"

#include <psp2/kernel/threadmgr.h>
#include <math.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include "utils/asset_cache.h"
#include "reimpl/controls.h"

#include "audio.h"

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

extern JNIEnv jni; 
so_module so_mod;
extern void *g_CMvApp_instance;

extern void settings_save();

int gameWidth = 480;
int gameHeight = 320;

int screenWidth = 960;
int screenHeight = 544;

int desiredFrameRate = 22;
uint64_t targetUs;
int currentSkillBank = 0;

// left 78.750000, 262.941193)
// right (183.750000, 263.529419
// down (125.750000, 317.352966
// up 154.000000, 220.588242
// action  (443.250000, 227.647064)
// jump (391.500000, 255.882370

void (*CMvApp_EvKeyPres)(void *this, void *event);
void (*CMvApp_EvKeyRelease)(void *this, void *event);
void (*CMvApp_EvPointerPress)(void *this, void *event);
void (*CMvApp_EvPointerRelease)(void *this, void *event);
void (*CMvApp_EvPointerMove)(void *this, void *event);
void (*CGsSound_GetSoundIDListIdx)(void *this, int soundID);
void (*CGsGraphics_DrawRect)(void *this, int x, int y, int w, int h, unsigned int color);

typedef enum {
    DPAD_NONE,
    DPAD_LEFT,
    DPAD_RIGHT,
    DPAD_UP,
    DPAD_DOWN,
    JUMP,
    ATTACK,
    SWITCHBANK,
    SKILL1,
    SKILL2,
    SKILL3,
    SKILL4,
    SKILL5,
    SKILL6,
    SKILL7,
    SKILL8,
    MAP,
    MENU
} virtual_buttons;

static virtual_buttons active_joystick_direction = DPAD_NONE;
static virtual_buttons active_dpad_direction = DPAD_NONE;

static const int virtual_dpad_coordinates[18][2] = {
    { 0, 0 },       // DPAD_NONE
    { 53,  253 },   // DPAD_LEFT
    { 140, 248 },   // DPAD_RIGHT
    { 107,  200 },   // DPAD_UP
    { 106,  290 },   // DPAD_DOWN
    { 376, 265 },    // JUMP
    { 438, 227 },   // ATTACK
    { 0, 0 },   // SWITCHBANK
    { 10, 60 },   // SKILL1
    { 60, 60 },   // SKILL2
    { 410, 60 },   // SKILL3
    { 460, 60 },   // SKILL4
    { 10, 125 },   // SKILL5
    { 60, 125 },   // SKILL6
    { 410, 125 },   // SKILL7
    { 460, 125 },   // SKILL8
    {38, 15}, // MAP
    {465, 12} // MENU
};

int main() {

    asset_cache_init(128, 4 * 1024 * 1024, 0);
    soloader_init_all();

    int (*JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    void (*InitializeJNIGlobalRefs)(void*, void*) = (void*)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef");
    void (*NativeInitDeviceInfo)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo");
    void (*NativeInitWithBufferSize)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize");
    void (*NativeResize)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeResize");
    void (*NativeRender)(void*, void*) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeRender");

    CMvApp_EvKeyPres = (void *)so_symbol(&so_mod, "_ZN6CMvApp10EvKeyPressEi");
    CMvApp_EvKeyRelease = (void *)so_symbol(&so_mod, "_ZN6CMvApp12EvKeyReleaseEi");
    CMvApp_EvPointerPress = (void *)so_symbol(&so_mod, "_ZN6CMvApp14EvPointerPressEP12GxPointerPos");
    CMvApp_EvPointerRelease = (void *)so_symbol(&so_mod, "_ZN6CMvApp16EvPointerReleaseEP12GxPointerPos");
    CMvApp_EvPointerMove = (void *)so_symbol(&so_mod, "_ZN6CMvApp13EvPointerMoveEP12GxPointerPos");

    CGsSound_GetSoundIDListIdx = (void *)so_symbol(&so_mod, "_ZN8CGsSound17GetSoundIDListIdxEi");
    CGsGraphics_DrawRect = (void *)so_symbol(&so_mod, "_ZN11CGsGraphics8DrawRectEiiiij");

    JNI_OnLoad(&jvm);

    FILE *configFile = fopen(DATA_PATH "config.txt", "r");
    if(!configFile){
        settings_save();
    }

    audio_init();
    gl_init();

    NativeInitWithBufferSize(&jvm, NULL, gameWidth, gameHeight);
    NativeInitDeviceInfo(&jvm, NULL, gameWidth, gameHeight);
    InitializeJNIGlobalRefs(&jni, NULL);
    NativeResize(&jvm, NULL, 960, 544);

    while (1) {
        controls_poll();

        uint64_t frameStart = sceKernelGetProcessTimeWide();
        NativeRender(&jvm, NULL);
        gl_swap();

        uint64_t elapsedUs = sceKernelGetProcessTimeWide() - frameStart;

        if (elapsedUs < targetUs) {
            sceKernelDelayThread((SceUInt)(targetUs - elapsedUs));
        }
    }

    sceKernelExitDeleteThread(0);
}

void controls_handler_key(int32_t keycode, ControlsAction action) {
    if (!g_CMvApp_instance) return;

    virtual_buttons btn = DPAD_NONE;
    if (keycode == AKEYCODE_DPAD_UP) btn = DPAD_UP;
    else if (keycode == AKEYCODE_DPAD_DOWN) btn = DPAD_DOWN;
    else if (keycode == AKEYCODE_DPAD_LEFT) btn = DPAD_LEFT;
    else if (keycode == AKEYCODE_DPAD_RIGHT) btn = DPAD_RIGHT;
    else if (keycode == AKEYCODE_BUTTON_A) btn = ATTACK;
    else if (keycode == AKEYCODE_BUTTON_B) btn = JUMP;
    else if (keycode == AKEYCODE_BUTTON_R1) btn = SWITCHBANK;
    else if (keycode == AKEYCODE_BUTTON_Y) btn = MENU;
    else if (keycode == AKEYCODE_BUTTON_START) btn = MAP;

    if (btn == DPAD_NONE) return;

    int touches[2] = { virtual_dpad_coordinates[btn][0], virtual_dpad_coordinates[btn][1] };

    switch (action) {
        case CONTROLS_ACTION_DOWN:
            if (btn == SWITCHBANK) {
                if(currentSkillBank == 0) {
                    currentSkillBank = 1;
                } else {
                    currentSkillBank = 0;
                }
            } else {
                if (btn == DPAD_UP || btn == DPAD_DOWN || btn == DPAD_LEFT || btn == DPAD_RIGHT) {
                    active_dpad_direction = btn;
                }
                CMvApp_EvPointerPress(g_CMvApp_instance, touches);
            }
            break;

        case CONTROLS_ACTION_MOVE:
            CMvApp_EvPointerMove(g_CMvApp_instance, touches);
            break;

        case CONTROLS_ACTION_UP:
            if (btn == active_dpad_direction) {
                active_dpad_direction = DPAD_NONE;
            }
            CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
            break;

        default: 
            break;
    }
}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action) {
    if (g_CMvApp_instance) {
        float xx = x * ((float)gameWidth / (float)screenWidth);
        float yy = y * ((float)gameHeight / (float)screenHeight);

        int touches[2] = { (int)xx, (int)yy };

        switch (action) {
            case CONTROLS_ACTION_DOWN:
                l_debug("controls_handler_touch: PointerPress at (%f, %f)", xx, yy);
                CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                break;
            case CONTROLS_ACTION_UP:
                l_debug("controls_handler_touch: PointerRelease at (%f, %f)", xx, yy);
                CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                break;
            case CONTROLS_ACTION_MOVE:
                l_debug("controls_handler_touch: PointerMove at (%f, %f)", xx, yy);
                CMvApp_EvPointerMove(g_CMvApp_instance, touches);
                break;
        }
    }
}

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action) {
    if (g_CMvApp_instance) {
        if (which == CONTROLS_STICK_LEFT) {

            virtual_buttons new_dpad_direction = DPAD_NONE;

            if (action != CONTROLS_ACTION_UP) {
                if (fabsf(x) > fabsf(y)) {
                    if (x < -0.5f) new_dpad_direction = DPAD_LEFT;
                    else if (x > 0.5f) new_dpad_direction = DPAD_RIGHT;
                } else {
                    if (y < -0.5f) new_dpad_direction = DPAD_UP;
                    else if (y > 0.5f) new_dpad_direction = DPAD_DOWN;
                }
            }

            if (new_dpad_direction == active_joystick_direction) {
                if (active_joystick_direction != DPAD_NONE) {
                    int touches[2] = { virtual_dpad_coordinates[active_joystick_direction][0], virtual_dpad_coordinates[active_joystick_direction][1] };
                    CMvApp_EvPointerMove(g_CMvApp_instance, touches);
                }
                return;
            }

            if (active_joystick_direction != DPAD_NONE) {
                int touches[2] = { virtual_dpad_coordinates[active_joystick_direction][0], virtual_dpad_coordinates[active_joystick_direction][1] };
                CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
            }

            if (new_dpad_direction != DPAD_NONE) {
                int touches[2] = { virtual_dpad_coordinates[new_dpad_direction][0], virtual_dpad_coordinates[new_dpad_direction][1] };
                CMvApp_EvPointerPress(g_CMvApp_instance, touches);
            }

            active_joystick_direction = new_dpad_direction;
        }

        if (which == CONTROLS_STICK_RIGHT) {

            virtual_buttons new_skill_direction = DPAD_NONE;

            if (action != CONTROLS_ACTION_UP) {
                if (fabsf(x) > fabsf(y)) {
                    if (x < -0.5f) {
                        if(currentSkillBank == 0) {
                            new_skill_direction = SKILL1;
                        } else {
                            new_skill_direction = SKILL3;
                        }
                    }  else if (x > 0.5f) {
                        if(currentSkillBank == 0) {
                            new_skill_direction = SKILL2;
                        } else {
                            new_skill_direction = SKILL4;
                        }
                    }
                } else {
                    if (y < -0.5f) {
                        if(currentSkillBank == 0) {
                            new_skill_direction = SKILL5;
                        } else {
                            new_skill_direction = SKILL7;
                        }
                    } else if (y > 0.5f) {
                        if(currentSkillBank == 0) {
                            new_skill_direction = SKILL6;
                        } else {
                            new_skill_direction = SKILL8;
                        }
                    }
                }
            }

            if(new_skill_direction != DPAD_NONE){
                int touches[2] = { virtual_dpad_coordinates[new_skill_direction][0], virtual_dpad_coordinates[new_skill_direction][1] };
                CMvApp_EvPointerPress(g_CMvApp_instance, touches);
            }
        }
    }
}