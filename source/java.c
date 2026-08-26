#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "reimpl/asset_manager.h"
#include "utils/logger.h"
#include "audio.h"

/*
 * JNI Methods
 */

static void stub_void(jmethodID id, va_list args) { (void)id; (void)args; }
static jint stub_int(jmethodID id, va_list args) { (void)id; (void)args; return 0; }
static jboolean stub_bool(jmethodID id, va_list args) { (void)id; (void)args; return JNI_FALSE; }
static jobject stub_obj(jmethodID id, va_list args) { (void)id; (void)args; return NULL; }
static jobject stub_bytearray(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)jda_alloc(0, FIELD_TYPE_BYTE);
}

static jobject readAssete_impl(jmethodID id, va_list args) {
    jstring jstr = va_arg(args, jstring);

    const char* filename = GetStringUTFChars(NULL, jstr, NULL);
    if (!filename) {
        return NULL;
    }

    sceClibPrintf("[readAssete] INFO: Asset open : %s\n", filename);

    AAsset* asset = AAssetManager_open(NULL, filename, AASSET_MODE_BUFFER);
    if (!asset) {
        ReleaseStringUTFChars(NULL, jstr, filename);
        return NULL;
    }

    int length = AAsset_getLength(asset);
    
    if (length <= 0) {
        AAsset_close(asset);
        ReleaseStringUTFChars(NULL, jstr, filename);
        return NULL;
    }

    jbyteArray jarr = NewByteArray(NULL, length);
    if (!jarr) {
        AAsset_close(asset);
        ReleaseStringUTFChars(NULL, jstr, filename);
        return NULL;
    }

    void* buffer = malloc(length);
    if (buffer) {
        AAsset_read(asset, buffer, length);
        SetByteArrayRegion(NULL, jarr, 0, length, (jbyte*)buffer);
        free(buffer);
    }

    AAsset_close(asset);
    ReleaseStringUTFChars(NULL, jstr, filename);

    return (jobject)jarr;
}

static jint isAssetExist_impl(jmethodID id, va_list args) {
    (void)id;

    jstring jstr = va_arg(args, jstring);
    
    if (!jstr) return 0;
    const char* filename = GetStringUTFChars(NULL, jstr, NULL);
    
    if (!filename) {
        return 0;
    }

    AAsset* asset = AAssetManager_open(NULL, filename, AASSET_MODE_UNKNOWN);
    int length = 0;
    
    if (asset) {
        length = AAsset_getLength(asset);
        AAsset_close(asset);
    } else {
        sceClibPrintf("[isAssetExist] ERROR: '%s' not found\n", filename);
    }

    ReleaseStringUTFChars(NULL, jstr, filename);
    
    return (jint)length;
}

static void OnSoundPlay_impl(jmethodID id, va_list args) {
    (void)id;

    return;

    jint sndID   = va_arg(args, jint);
    jint vol     = va_arg(args, jint);
    jboolean isLoop = (jboolean)va_arg(args, jint);

    char filepath[256];
    audio_play_sound(sndID, vol, isLoop);
}

static void OnStopSound_imp(jmethodID id, va_list args) {
    (void)id;

        return;

    sceClibPrintf("[OnStopSound] Stopping all sounds\n");
    audio_stop_sound();
}

static int SetSpeed_imp(jmethodID id, va_list args) {
    (void)id;

    jint speed = va_arg(args, jint);
    sceClibPrintf("[SetSpeed] Setting speed to %d\n", speed);

    return 1;
}

jobject getAbsolueFilePath_impl(jmethodID id, va_list args) {
    (void)id;
    
    sceClibPrintf("[getAbsolueFilePath] getting the path?\n");
    return NewStringUTF(&jni, DATA_PATH);
}

jobject getPhoneNumber_impl(jmethodID id, va_list args) {
    (void)va_arg(args, jobject);

    const char *fake_number = "5555555555";
    jsize len = (jsize)strlen(fake_number);

    JavaDynArray *jda = jda_alloc(len, FIELD_TYPE_BYTE);
    if (!jda) {
        sceClibPrintf("[getPhoneNumber] alloc failed\n");
        return NULL;
    }

    memcpy(jda->array, fake_number, len);

    sceClibPrintf("[getPhoneNumber] stubbed, returning fake number\n");
    return (jobject)jda;
}

jobject getPhoneModel_impl(jmethodID id, va_list args) {
    (void)va_arg(args, jobject);

    const char *fake_model = "PSVita";
    jsize len = (jsize)strlen(fake_model);

    JavaDynArray *jda = jda_alloc(len, FIELD_TYPE_BYTE);
    if (!jda) {
        sceClibPrintf("[getPhoneModel] alloc failed\n");
        return NULL;
    }

    memcpy(jda->array, fake_model, len);

    sceClibPrintf("[getPhoneModel] stubbed, returning fake model\n");
    return (jobject)jda;
}

NameToMethodID nameToMethodId[] = {

    { 100, "getAbsolueFilePath", METHOD_TYPE_OBJECT },
    { 101, "OnSoundPlay", METHOD_TYPE_VOID },
    { 102, "OnStopSound", METHOD_TYPE_VOID },
    { 103, "OnVibrate", METHOD_TYPE_VOID },
    { 104, "SetSpeed", METHOD_TYPE_INT },
    { 105, "isAssetExist", METHOD_TYPE_INT },
    { 106, "readAssete", METHOD_TYPE_OBJECT },
    { 107, "getPhoneNumber", METHOD_TYPE_OBJECT },
    { 108, "getLanguage", METHOD_TYPE_VOID },
    { 109, "getPhoneModel", METHOD_TYPE_OBJECT }
};

MethodsBoolean methodsBoolean[] = {
    { 100, stub_bool },
    { 101, stub_bool },
    { 102, stub_bool },
    { 103, stub_bool },
    { 104, stub_bool },
    { 105, stub_bool },
    { 106, stub_bool },
    { 107, stub_bool },
    { 108, stub_bool },
    { 109, stub_bool }
};
MethodsByte methodsByte[] = {
    { 100, stub_void },
    { 101, stub_void },
    { 102, stub_void },
    { 103, stub_void },
    { 104, stub_void },
    { 105, stub_void },
    { 106, stub_void },
    { 107, stub_void },
    { 108, stub_void },
    { 109, stub_void }
};
MethodsChar methodsChar[] = {
    { 100, stub_void },
    { 101, stub_void },
    { 102, stub_void },
    { 103, stub_void },
    { 104, stub_void },
    { 105, stub_void },
    { 106, stub_void },
    { 107, stub_void },
    { 108, stub_void },
    { 109, stub_void }
};
MethodsDouble methodsDouble[] = {
    { 100, stub_int },
    { 101, stub_int },
    { 102, stub_int },
    { 103, stub_int },
    { 104, stub_int },
    { 105, stub_int },
    { 106, stub_int },
    { 107, stub_int },
    { 108, stub_int },
    { 109, stub_int}
};
MethodsFloat methodsFloat[] = {
    { 100, stub_int },
    { 101, stub_int },
    { 102, stub_int },
    { 103, stub_int },
    { 104, stub_int },
    { 105, stub_int },
    { 106, stub_int },
    { 107, stub_int },
    { 108, stub_int },
    { 109, stub_int }
};
MethodsInt methodsInt[] = {
    { 100, stub_int },
    { 101, stub_int },
    { 102, stub_int },
    { 103, stub_int },
    { 104, SetSpeed_imp },
    { 105, isAssetExist_impl },
    { 106, stub_int },
    { 107, stub_int },
    { 108, stub_int },
    { 109, stub_int }
};
MethodsLong methodsLong[] = {
    { 100, stub_int },
    { 101, stub_int },
    { 102, stub_int },
    { 103, stub_int },
    { 104, stub_int },
    { 105, stub_int },
    { 106, stub_int },
    { 107, stub_int },
    { 108, stub_int },
    { 109, stub_int }
};
MethodsObject methodsObject[] = {
    { 100, getAbsolueFilePath_impl },
    { 101, stub_obj },
    { 102, stub_obj },
    { 103, stub_obj },
    { 104, stub_obj },
    { 105, stub_obj },
    { 106, readAssete_impl },
    { 107, getPhoneNumber_impl },
    { 108, stub_obj },
    { 109, getPhoneModel_impl }
};
MethodsShort methodsShort[] = {
    { 100, stub_void },
    { 101, stub_void },
    { 102, stub_void },
    { 103, stub_void },
    { 104, stub_void },
    { 105, stub_void },
    { 106, stub_void },
    { 107, stub_void },
    { 108, stub_void },
    { 109, stub_void }
};
MethodsVoid methodsVoid[] = {
    { 100, stub_void },
    { 101, OnSoundPlay_impl },
    { 102, OnStopSound_imp},
    { 103, stub_void},
    { 104, stub_void },
    { 105, stub_void },
    { 106, stub_void },
    { 107, stub_void },
    { 108, stub_void },
    { 109, stub_void }
};

/*
 * JNI Fields
 */

// System-wide constant that applications sometimes request
// https://developer.android.com/reference/android/content/Context.html#WINDOW_SERVICE
char WINDOW_SERVICE[] = "window";

// System-wide constant that's often used to determine Android version
// https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT
// Possible values: https://developer.android.com/reference/android/os/Build.VERSION_CODES
const int SDK_INT = 19; // Android 4.4 / KitKat

NameToFieldID nameToFieldId[] = {
	{ 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT },
	{ 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
	{ 1, SDK_INT },
};
FieldsObject fieldsObject[] = {
	{ 0, WINDOW_SERVICE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
