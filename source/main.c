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
extern void settings_save();

int gameWidth = 320;
int gameHeight = 240;

int screenWidth = 960;
int screenHeight = 544;

int main() {

    asset_cache_init(128, 4 * 1024 * 1024, 0);
    soloader_init_all();

    int (*JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    void (*InitializeJNIGlobalRefs)(void*, void*) = (void*)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef");
    void (*NativeInitDeviceInfo)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo");
    void (*NativeInitWithBufferSize)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize");
    void (*NativeResize)(void*, void*, int, int) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeResize");
    void (*NativeRender)(void*, void*) = (void *)so_symbol(&so_mod, "Java_com_gamevil_nexus2_Natives_NativeRender");

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

        NativeRender(&jvm, NULL);
        gl_swap();
    }

    sceKernelExitDeleteThread(0);
}

void controls_handler_key(int32_t keycode, ControlsAction action) {

}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action) {

}

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action) {

}