/* DO NOT EDIT THIS FILE - it is machine generated */
#include <JavaVM/jni.h>
/* Header for class com_portaudio_PortAudio */

#ifndef _Included_com_portaudio_PortAudio
#define _Included_com_portaudio_PortAudio
#ifdef __cplusplus
extern "C" {
#endif
#undef com_portaudio_PortAudio_FLAG_CLIP_OFF
#define com_portaudio_PortAudio_FLAG_CLIP_OFF 1L
#undef com_portaudio_PortAudio_FLAG_DITHER_OFF
#define com_portaudio_PortAudio_FLAG_DITHER_OFF 2L
#undef com_portaudio_PortAudio_FORMAT_FLOAT_32
#define com_portaudio_PortAudio_FORMAT_FLOAT_32 1L
#undef com_portaudio_PortAudio_FORMAT_INT_32
#define com_portaudio_PortAudio_FORMAT_INT_32 2L
#undef com_portaudio_PortAudio_FORMAT_INT_24
#define com_portaudio_PortAudio_FORMAT_INT_24 4L
#undef com_portaudio_PortAudio_FORMAT_INT_16
#define com_portaudio_PortAudio_FORMAT_INT_16 8L
#undef com_portaudio_PortAudio_FORMAT_INT_8
#define com_portaudio_PortAudio_FORMAT_INT_8 16L
#undef com_portaudio_PortAudio_FORMAT_UINT_8
#define com_portaudio_PortAudio_FORMAT_UINT_8 32L
#undef com_portaudio_PortAudio_HOST_API_TYPE_DEV
#define com_portaudio_PortAudio_HOST_API_TYPE_DEV 0L
#undef com_portaudio_PortAudio_HOST_API_TYPE_DIRECTSOUND
#define com_portaudio_PortAudio_HOST_API_TYPE_DIRECTSOUND 1L
#undef com_portaudio_PortAudio_HOST_API_TYPE_MME
#define com_portaudio_PortAudio_HOST_API_TYPE_MME 2L
#undef com_portaudio_PortAudio_HOST_API_TYPE_ASIO
#define com_portaudio_PortAudio_HOST_API_TYPE_ASIO 3L
#undef com_portaudio_PortAudio_HOST_API_TYPE_SOUNDMANAGER
#define com_portaudio_PortAudio_HOST_API_TYPE_SOUNDMANAGER 4L
#undef com_portaudio_PortAudio_HOST_API_TYPE_COREAUDIO
#define com_portaudio_PortAudio_HOST_API_TYPE_COREAUDIO 5L
#undef com_portaudio_PortAudio_HOST_API_TYPE_OSS
#define com_portaudio_PortAudio_HOST_API_TYPE_OSS 7L
#undef com_portaudio_PortAudio_HOST_API_TYPE_ALSA
#define com_portaudio_PortAudio_HOST_API_TYPE_ALSA 8L
#undef com_portaudio_PortAudio_HOST_API_TYPE_AL
#define com_portaudio_PortAudio_HOST_API_TYPE_AL 9L
#undef com_portaudio_PortAudio_HOST_API_TYPE_BEOS
#define com_portaudio_PortAudio_HOST_API_TYPE_BEOS 10L
#undef com_portaudio_PortAudio_HOST_API_TYPE_WDMKS
#define com_portaudio_PortAudio_HOST_API_TYPE_WDMKS 11L
#undef com_portaudio_PortAudio_HOST_API_TYPE_JACK
#define com_portaudio_PortAudio_HOST_API_TYPE_JACK 12L
#undef com_portaudio_PortAudio_HOST_API_TYPE_WASAPI
#define com_portaudio_PortAudio_HOST_API_TYPE_WASAPI 13L
#undef com_portaudio_PortAudio_HOST_API_TYPE_AUDIOSCIENCE
#define com_portaudio_PortAudio_HOST_API_TYPE_AUDIOSCIENCE 14L
#undef com_portaudio_PortAudio_HOST_API_TYPE_COUNT
#define com_portaudio_PortAudio_HOST_API_TYPE_COUNT 15L
/*
 * Class:     com_portaudio_PortAudio
 * Method:    getVersion
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_getVersion
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getVersionText
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_portaudio_PortAudio_getVersionText
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_portaudio_PortAudio_initialize
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    terminate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_portaudio_PortAudio_terminate
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getDeviceCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_getDeviceCount
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getDeviceInfo
 * Signature: (ILcom/portaudio/DeviceInfo;)V
 */
JNIEXPORT void JNICALL Java_com_portaudio_PortAudio_getDeviceInfo
  (JNIEnv *, jclass, jint, jobject);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getHostApiCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_getHostApiCount
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getHostApiInfo
 * Signature: (ILcom/portaudio/HostApiInfo;)V
 */
JNIEXPORT void JNICALL Java_com_portaudio_PortAudio_getHostApiInfo
  (JNIEnv *, jclass, jint, jobject);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    hostApiTypeIdToHostApiIndex
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_hostApiTypeIdToHostApiIndex
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    hostApiDeviceIndexToDeviceIndex
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_hostApiDeviceIndexToDeviceIndex
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getDefaultInputDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_getDefaultInputDevice
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getDefaultOutputDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_getDefaultOutputDevice
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    getDefaultHostApi
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_getDefaultHostApi
  (JNIEnv *, jclass);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    isFormatSupported
 * Signature: (Lcom/portaudio/StreamParameters;Lcom/portaudio/StreamParameters;I)I
 */
JNIEXPORT jint JNICALL Java_com_portaudio_PortAudio_isFormatSupported
  (JNIEnv *, jclass, jobject, jobject, jint);

/*
 * Class:     com_portaudio_PortAudio
 * Method:    openStream
 * Signature: (Lcom/portaudio/BlockingStream;Lcom/portaudio/StreamParameters;Lcom/portaudio/StreamParameters;III)V
 */
JNIEXPORT void JNICALL Java_com_portaudio_PortAudio_openStream
  (JNIEnv *, jclass, jobject, jobject, jobject, jint, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
