#include <stdio.h>
#include "utils/init.h"
#include "utils/glutil.h"

#include <psp2/kernel/threadmgr.h>
#include <math.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#include "reimpl/controls.h"

#include "audio.h"

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

so_module so_mod;
extern void *g_CMvApp_instance;
extern void *g_CSaveMgr_instance;
extern void settings_save();

int gameHeight = 320;
int gameWidth = 480;
int screenHeight = 544;
int screenWidth = 960;

int desiredFrameRate = 22; // game speed slider supports the following: 10, 13, 16, 19, and 22 fps. default is going to cap at 22.
uint64_t targetUs;

void (*CMvApp_EvKeyPres)(void *this, void *event);
void (*CMvApp_EvKeyRelease)(void *this, void *event);
void (*CMvApp_EvPointerPress)(void *this, void *event);
void (*CMvApp_EvPointerRelease)(void *this, void *event);
void (*CMvApp_EvPointerMove)(void *this, void *event);

typedef enum {
    DPAD_NONE,
    DPAD_LEFT,
    DPAD_RIGHT,
    DPAD_UP,
    DPAD_DOWN,
    SKILL_1,
    SKILL_2,
    SKILL_3,
    SKILL_4,
    TEAM_ATTACK,
    ATTACK
} virtual_buttons;

static virtual_buttons current_dpad_direction = DPAD_NONE;

static const int virtual_dpad_coordinates[11][2] = {
    { 0, 0 },       // DPAD_NONE
    { 37,  252 },   // DPAD_LEFT
    { 128, 262 },   // DPAD_RIGHT
    { 86,  207 },   // DPAD_UP
    { 64,  293 },   // DPAD_DOWN
    { 195, 288 },   // SKILL_1
    { 242, 288 },   // SKILL_2
    { 283, 288 },   // SKILL_3
    { 328, 288 },   // SKILL_4
    { 36, 127 },    // TEAM_ATTACK
    { 428, 259 },   // ATTACK
};

int main() {

    soloader_init_all();

    int (*JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    void (*InitializeJNIGlobalRefs)(void*, void*) = (void*)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef");
    void (*NativeInitDeviceInfo)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo");
    void (*NativeInitWithBufferSize)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize");
    void (*NativeResize)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeResize");
    void (*NativeRender)(void*, void*) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeRender");

    void (*CMvApp_InitialTouchPoint)(void*) = (void *)so_symbol(&so_mod, "_ZN6CMvApp17InitialTouchPointEv");
    void (*CSaveMgr_SetGameQuality)(void*, int) = (void *)so_symbol(&so_mod, "_ZN8CSaveMgr14SetGameQualityEi");
    void (*CSaveMgr_SetOutLine)(void*, int) = (void *)so_symbol(&so_mod, "_ZN8CSaveMgr10SetOutLineEi");

    CMvApp_EvKeyPres = (void *)so_symbol(&so_mod, "_ZN6CMvApp10EvKeyPressEi");
    CMvApp_EvKeyRelease = (void *)so_symbol(&so_mod, "_ZN6CMvApp12EvKeyReleaseEi");
    CMvApp_EvPointerPress = (void *)so_symbol(&so_mod, "_ZN6CMvApp14EvPointerPressEP12GxPointerPos");
    CMvApp_EvPointerRelease = (void *)so_symbol(&so_mod, "_ZN6CMvApp16EvPointerReleaseEP12GxPointerPos");
    CMvApp_EvPointerMove = (void *)so_symbol(&so_mod, "_ZN6CMvApp13EvPointerMoveEP12GxPointerPos");

    targetUs = 1000000ULL / desiredFrameRate;

    JNI_OnLoad(&jvm);

    FILE *configFile = fopen(DATA_PATH "config.txt", "r");
    if(!configFile){
        settings_save();
    }

    audio_init();
    gl_init();
    eglSwapInterval(0, 2);

    NativeInitWithBufferSize(&jvm, NULL, gameWidth, gameHeight);
    NativeInitDeviceInfo(&jvm, NULL, gameWidth, gameHeight);
    InitializeJNIGlobalRefs(&jni, NULL);
    NativeResize(&jvm, NULL, 960, 544);

    if(g_CMvApp_instance){
        CMvApp_InitialTouchPoint(g_CMvApp_instance);
    }

    l_debug("CSaveMgr_SetGameQuality");
    if(g_CSaveMgr_instance){
        l_debug("CSaveMgr_SetGameQuality setting to 0");
        CSaveMgr_SetGameQuality(g_CSaveMgr_instance, 0);
        CSaveMgr_SetOutLine(g_CSaveMgr_instance, 0);
    }

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
    if (g_CMvApp_instance) {
        int32_t avk = vita_to_control(keycode);

        switch (action) {
            case CONTROLS_ACTION_DOWN:
                if(keycode == AKEYCODE_BUTTON_A) {
                    int touches[2] = { virtual_dpad_coordinates[ATTACK][0], virtual_dpad_coordinates[ATTACK][1] };
                    CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_UP){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_1][0], virtual_dpad_coordinates[SKILL_1][1] };
                    CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_DOWN){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_3][0], virtual_dpad_coordinates[SKILL_3][1] };
                    CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_LEFT){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_4][0], virtual_dpad_coordinates[SKILL_4][1] };
                    CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_RIGHT){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_2][0], virtual_dpad_coordinates[SKILL_2][1] };
                    CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_BUTTON_Y){
                    int touches[2] = { virtual_dpad_coordinates[TEAM_ATTACK][0], virtual_dpad_coordinates[TEAM_ATTACK][1] };
                    CMvApp_EvPointerPress(g_CMvApp_instance, touches);
                } else {
                    CMvApp_EvKeyPres(g_CMvApp_instance, avk);
                }
                break;
            case CONTROLS_ACTION_UP:
                if(keycode == AKEYCODE_BUTTON_A) {
                    int touches[2] = { virtual_dpad_coordinates[ATTACK][0], virtual_dpad_coordinates[ATTACK][1] };
                    CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_UP){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_1][0], virtual_dpad_coordinates[SKILL_1][1] };
                    CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_DOWN){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_3][0], virtual_dpad_coordinates[SKILL_3][1] };
                    CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_LEFT){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_4][0], virtual_dpad_coordinates[SKILL_4][1] };
                    CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_DPAD_RIGHT){
                    int touches[2] = { virtual_dpad_coordinates[SKILL_2][0], virtual_dpad_coordinates[SKILL_2][1] };
                    CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                } else if (keycode == AKEYCODE_BUTTON_Y){
                    int touches[2] = { virtual_dpad_coordinates[TEAM_ATTACK][0], virtual_dpad_coordinates[TEAM_ATTACK][1] };
                    CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
                } else {
                    CMvApp_EvKeyRelease(g_CMvApp_instance, avk);
                }
                break; 
        }
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

            if (new_dpad_direction == current_dpad_direction) {
                if (current_dpad_direction != DPAD_NONE) {
                    int touches[2] = { virtual_dpad_coordinates[current_dpad_direction][0], virtual_dpad_coordinates[current_dpad_direction][1] };
                    CMvApp_EvPointerMove(g_CMvApp_instance, touches);
                }
                return;
            }

            if (current_dpad_direction != DPAD_NONE) {
                int touches[2] = { virtual_dpad_coordinates[current_dpad_direction][0], virtual_dpad_coordinates[current_dpad_direction][1] };
                CMvApp_EvPointerRelease(g_CMvApp_instance, touches);
            }

            if (new_dpad_direction != DPAD_NONE) {
                int touches[2] = { virtual_dpad_coordinates[new_dpad_direction][0], virtual_dpad_coordinates[new_dpad_direction][1] };
                CMvApp_EvPointerPress(g_CMvApp_instance, touches);
            }

            current_dpad_direction = new_dpad_direction;
        }
    }
}

int32_t vita_to_control(int32_t vita_button) {
    switch (vita_button) {
        /*
        -16	-- menu
        -14	26
        -13	25
        -12	-- change skill bar
        -11	21
        -10	20
        -8	19
        -7	18
        -6	17
        -5	16
        -4	-- jump right
        -3	-- jump left
        -2	-- change player
        -1	12

        // this isnt a great mapping since not all of the buttons are mapped properly. 

        */
        case AKEYCODE_BUTTON_L1: return -2; 
        case AKEYCODE_BUTTON_R1: return -12; 
        case AKEYCODE_BUTTON_X: return -3;
        case AKEYCODE_BUTTON_B: return -4;
        case AKEYCODE_BUTTON_A: return -5;
        case AKEYCODE_BUTTON_START: return -16;
        case AKEYCODE_BUTTON_SELECT: return -10;

        case AKEYCODE_DPAD_UP: return 0;
        case AKEYCODE_DPAD_DOWN: return -6;
        case AKEYCODE_DPAD_RIGHT: return -7;
        case AKEYCODE_DPAD_LEFT: return -8;
        
        default: return 0;
    }
}