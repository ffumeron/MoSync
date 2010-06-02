/* Copyright (C) 2010 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include "config_platform.h"
#include "Syscall.h"
#include "Core.h"

#include <helpers/cpp_defs.h>
#include <helpers/fifo.h>

#include <jni.h>

#define ERROR_EXIT { MoSyncErrorExit(-1); }

#define SYSLOG(a) __android_log_write(ANDROID_LOG_INFO, "JNI Syscalls", a);
//#define SYSLOG(...)

namespace Base
{
	Syscall* gSyscall;

	JNIEnv* mJNIEnv;
	jobject mJThis;
	
	static ResourceArray gResourceArray;
	static CircularFifo<MAEvent, EVENT_BUFFER_SIZE> gEventFifo;
	
	int mClipLeft, mClipTop, mClipWidth, mClipHeight;
	MAHandle drawTargetHandle = HANDLE_SCREEN;
	
	Syscall::Syscall()
	{
		gSyscall = this;
	}

	bool Syscall::loadImage(int resourceIndex, int pos, int length)
	{
		SYSLOG("loadImage");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "loadImage", "(III)Z");
		if (methodID == 0) ERROR_EXIT;
		bool retVal = mJNIEnv->CallBooleanMethod(mJThis, methodID, resourceIndex, pos, length);
				
		mJNIEnv->DeleteLocalRef(cls);
		return retVal;
	}
	
	Image* Syscall::loadSprite(void* surface, ushort left, ushort top, ushort width, ushort height, ushort cx, ushort cy)
	{
		SYSLOG("loadSprite - NOT IMPLEMENTED");
		
		return NULL;
	}
	
	char* Syscall::loadBinary(int resourceIndex, int size)
	{
		SYSLOG("loadBinary");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "loadBinary", "(II)Ljava/nio/ByteBuffer;");
		if (methodID == 0) return NULL;
		jobject jo = mJNIEnv->CallObjectMethod(mJThis, methodID, resourceIndex, size);
		char* buffer = (char*)mJNIEnv->GetDirectBufferAddress(jo);
		mJNIEnv->DeleteLocalRef(cls);
		
		return buffer;
	}
	
	void Syscall::platformDestruct()
	{
	
	}

	void Syscall::setJNIEnvironment(JNIEnv* je, jobject jthis)
	{
		SYSLOG("setJNIEnvironment")
		
		mJNIEnv = je;
		mJThis = jthis;
	}
	
	void Syscall::postEvent(MAEvent event)
	{
		SYSLOG("PostEvent");
		gEventFifo.put(event);
	}
	
	SYSCALL(int,  maSetColor(int rgb))
	{
		SYSLOG("maSetColor");
	
		if(rgb<=0xffffff) rgb += 0xff000000;
	
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maSetColor", "(I)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, rgb);
		
		mJNIEnv->DeleteLocalRef(cls);
	
		return retval;
	}

	SYSCALL(void,  maSetClipRect(int left, int top, int width, int height))
	{
		SYSLOG("maSetClipRect");
		
		mClipLeft = left;
		mClipTop = top;
		mClipWidth = width;
		mClipHeight = height;
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maSetClipRect", "(IIII)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, left, top, width, height);
		
		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(void,  maGetClipRect(MARect* rect))
	{
		SYSLOG("maGetClipRect");
		
		gSyscall->ValidateMemRange(rect, sizeof(MARect));		
		rect->left = mClipLeft;
		rect->top = mClipTop;
		rect->width = mClipWidth;
		rect->height = mClipHeight;
	}

	SYSCALL(void,  maPlot(int posX, int posY))
	{
		SYSLOG("maPlot");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maSetColor", "(II)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, posX, posY);
		
		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(void,  maLine(int startX, int startY, int endX, int endY))
	{
		SYSLOG("maLine");

		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maLine", "(IIII)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, startX, startY, endX, endY);
		
		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(void,  maFillRect(int left, int top, int width, int height))
	{
		SYSLOG("maFillRect");

		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maFillRect", "(IIII)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, left, top, width, height);
		
		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(void,  maFillTriangleStrip(const MAPoint2d* points, int count))
	{
		SYSLOG("maFillTriangleStrip NOT IMPLEMENTED");

	}

	SYSCALL(void,  maFillTriangleFan(const MAPoint2d* points, int count))
	{
		SYSLOG("maFillTriangleFan NOT IMPLEMENTED");

	}

	SYSCALL(MAExtent,  maGetTextSize(const char* str))
	{
		SYSLOG("maGetTextSize");
		
		int l = strlen(str);
		char* b = (char*)malloc(l+100);
		sprintf(b, "len: %i str: %s\n", l, str);
		SYSLOG(b);
		free(b);
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maGetTextSize", "(Ljava/lang/String;)I");
		if (methodID == 0) ERROR_EXIT;
		jstring jstr = mJNIEnv->NewStringUTF(str);
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, jstr);
		
		mJNIEnv->DeleteLocalRef(cls);
		mJNIEnv->DeleteLocalRef(jstr);
	
		return retval;
	}
	
	int wcharLength(const wchar* str)
	{
		int l = 0;
		while(str[l] != 0) l++;
		return l;
	}

	SYSCALL(MAExtent,  maGetTextSizeW(const wchar* str))
	{
		SYSLOG("maGetTextSizeW");
		
		jsize len = wcharLength(str);
		jstring jstr = mJNIEnv->NewString((jchar*)str, len);

		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maGetTextSizeW", "(Ljava/lang/String;)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, jstr);
		
		mJNIEnv->DeleteLocalRef(cls);
		mJNIEnv->DeleteLocalRef(jstr);
		
		return retval;
	}

	SYSCALL(void,  maDrawText(int left, int top, const char* str))
	{
		SYSLOG("maDrawText");
		
		int l = strlen(str);
		char* b = (char*)malloc(l+100);
		sprintf(b, "str: %s\n", str);
		SYSLOG(b);
		free(b);
		
		jstring jstr = mJNIEnv->NewStringUTF(str);
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maDrawText", "(IILjava/lang/String;)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, left, top, jstr);
		mJNIEnv->DeleteLocalRef(cls);
		mJNIEnv->DeleteLocalRef(jstr);
	}

	SYSCALL(void,  maDrawTextW(int left, int top, const wchar* str))
	{
		SYSLOG("maDrawTextW");
		
		jsize len = wcharLength(str);
		jstring jstr = mJNIEnv->NewString((jchar*)str, len);

		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maDrawTextW", "(IILjava/lang/String;)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, left, top, jstr);
		mJNIEnv->DeleteLocalRef(cls);
		mJNIEnv->DeleteLocalRef(jstr);
		
	}

	SYSCALL(void,  maUpdateScreen(void))
	{
		SYSLOG("maUpdateScreen");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);		
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maUpdateScreen", "()V");	
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID);

		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(void,  maResetBacklight(void))
	{
		SYSLOG("maResetBacklight NOT IMPLEMENTED");
	}

	SYSCALL(MAExtent,  maGetScrSize(void))
	{
		SYSLOG("maGetScrSize");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maGetScrSize", "()I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(void,  maDrawImage(MAHandle image, int left, int top))
	{
		SYSLOG("maDrawImage");

		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maDrawImage", "(III)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, image, left, top);
		
		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(void,  maDrawRGB(const MAPoint2d* dstPoint, const void* src, const MARect* srcRect, int scanlength))
	{
		SYSLOG("maDrawRGB NOT IMPLEMENTED");

	}

	SYSCALL(void,  maDrawImageRegion(MAHandle image, const MARect* srcRect, const MAPoint2d* dstPoint, int transformMode))
	{
		SYSLOG("maDrawImageRegion");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "_maDrawImageRegion", "(IIIIIIII)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, image, srcRect->left, srcRect->top, srcRect->width, srcRect->height, dstPoint->x, dstPoint->y, transformMode);
		
		mJNIEnv->DeleteLocalRef(cls);
		
	}

	SYSCALL(MAExtent,  maGetImageSize(MAHandle image))
	{
		SYSLOG("maGetImageSize");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maGetImageSize", "(I)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, image);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(void,  maGetImageData(MAHandle image, void* dst, const MARect* srcRect, int scanlength))
	{
		SYSLOG("maGetImageData NOT IMPLEMENTED");
		
		
	}

	SYSCALL(MAHandle,  maSetDrawTarget(MAHandle image))
	{
		SYSLOG("maSetDrawTarget");
		
		MAHandle temp = drawTargetHandle;
		int currentDrawSurface;
		
		if(drawTargetHandle != HANDLE_SCREEN)
		{
			SYSCALL_THIS->resources.extract_RT_FLUX(drawTargetHandle);
			if(SYSCALL_THIS->resources.add_RT_IMAGE(drawTargetHandle, NULL) == RES_OUT_OF_MEMORY)
			{
				maPanic(ERR_RES_OOM, "maSetDrawTarget couldn't allocate drawtarget");
			}
			drawTargetHandle = HANDLE_SCREEN;
		}
		
		if(image == HANDLE_SCREEN)
		{
			currentDrawSurface = HANDLE_SCREEN;
		} 
		else
		{
			currentDrawSurface = image;
			SYSCALL_THIS->resources.extract_RT_IMAGE(image);
			if(SYSCALL_THIS->resources.add_RT_FLUX(image, NULL) == RES_OUT_OF_MEMORY)
			{
				maPanic(ERR_RES_OOM, "maSetDrawTarget couldn't allocate drawtarget");
			}
		}
		drawTargetHandle = image;
			
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maSetDrawTarget", "(I)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, currentDrawSurface);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return temp;
	}

	SYSCALL(int,  maCreateImageFromData(MAHandle placeholder, MAHandle data, int offset, int size))
	{
		SYSLOG("maCreateImageFromData");
		
		char* b = (char*)malloc(200);
		sprintf(b, "placeholder: %i data: %i\n", placeholder, data);
		SYSLOG(b);
		
		if(SYSCALL_THIS->resources.add_RT_IMAGE(placeholder, NULL) == RES_OUT_OF_MEMORY) return RES_OUT_OF_MEMORY;
		placeholder = placeholder&(~DYNAMIC_PLACEHOLDER_BIT);
		
		
		sprintf(b, "new placeholder: %i\n", placeholder);
		SYSLOG(b);
		free(b);
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maCreateImageFromData", "(IIII)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, placeholder, data, offset, size);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(int,  maCreateImageRaw(MAHandle placeholder, const void* src, MAExtent size, int alpha))
	{
		SYSLOG("maCreateImageRaw NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(int,  maCreateDrawableImage(MAHandle placeholder, int width, int height))
	{
		SYSLOG("maCreateDrawableImage");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maCreateDrawableImage", "(III)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, placeholder, width, height);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		if(RES_OK == retval)
			return SYSCALL_THIS->resources.add_RT_IMAGE(placeholder, NULL);
		
		return retval;
	}

	SYSCALL(MAHandle,  maOpenStore(const char* name, int flags))
	{
		SYSLOG("maOpenStore");
		
		jstring jname = mJNIEnv->NewStringUTF(name);
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maOpenStore", "(Ljava/lang/String;I)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, jname, flags);
		
		mJNIEnv->DeleteLocalRef(cls);
		mJNIEnv->DeleteLocalRef(jname);
		
		return retval;
	}

	SYSCALL(int,  maWriteStore(MAHandle store, MAHandle data))
	{
		SYSLOG("maWriteStore");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maWriteStore", "(II)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, store, data);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(int,  maReadStore(MAHandle store, MAHandle placeholder))
	{
		SYSLOG("maReadStore");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maReadStore", "(II)I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID, store, placeholder);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(void,  maCloseStore(MAHandle store, int remove))
	{
		SYSLOG("maCloseStore");

		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maCloseStore", "(II)V");
		if (methodID == 0) ERROR_EXIT;
		mJNIEnv->CallVoidMethod(mJThis, methodID, store, remove);
		
		mJNIEnv->DeleteLocalRef(cls);
	}

	SYSCALL(MAHandle,  maConnect(const char* url))
	{
		SYSLOG("maConnect NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(void,  maConnClose(MAHandle conn))
	{
		SYSLOG("maConnClose NOT IMPLEMENTED");

	}

	SYSCALL(void,  maConnRead(MAHandle conn, void* dst, int size))
	{
		SYSLOG("maConnRead NOT IMPLEMENTED");

	}

	SYSCALL(void,  maConnWrite(MAHandle conn, const void* src, int size))
	{
		SYSLOG("maConnWrite NOT IMPLEMENTED");

	}

	SYSCALL(void,  maConnReadToData(MAHandle conn, MAHandle data, int offset, int size))
	{
		SYSLOG("maConnReadToData NOT IMPLEMENTED");

	}

	SYSCALL(void,  maConnWriteFromData(MAHandle conn, MAHandle data, int offset, int size))
	{
		SYSLOG("maConnWriteFromData NOT IMPLEMENTED");

	}

	SYSCALL(int,  maConnGetAddr(MAHandle conn, MAConnAddr* addr))
	{
		SYSLOG("maConnGetAddr NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(MAHandle,  maHttpCreate(const char* url, int method))
	{
		SYSLOG("mahttpCreate NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(void,  maHttpSetRequestHeader(MAHandle conn, const char* key, const char* value))
	{
		SYSLOG("maHttpSetRequestHeader NOT IMPLEMENTED");

	}

	SYSCALL(int,  maHttpGetResponseHeader(MAHandle conn, const char* key, char* buffer, int bufSize))
	{
		SYSLOG("maHttpGetResponseHeader NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(void,  maHttpFinish(MAHandle conn))
	{
		SYSLOG("maHttpFinish NOT IMPLEMENTED");

	}

	SYSCALL(void,  maLoadProgram(MAHandle data, int reload))
	{
		SYSLOG("maLoadProgram NOT IMPLEMENTED");

	}

	SYSCALL(int,  maGetKeys(void))
	{
		SYSLOG("maGetKeys NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(int,  maGetEvent(MAEvent* event))
	{
		gSyscall->ValidateMemRange(event, sizeof(MAEvent));	
		MYASSERT(((uint)event & 3) == 0, ERR_MEMORY_ALIGNMENT);	//alignment
		
		if(gEventFifo.count() == 0)
			return 0;
		
//		SYSLOG("maGetEvent");

		*event = gEventFifo.get();
		
		return 1;
	}

	SYSCALL(void,  maWait(int timeout))
	{
//		SYSLOG("maWait NOT IMPLEMENTED");
	}

	SYSCALL(int,  maTime(void))
	{
		SYSLOG("maTime");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maTime", "()I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(int,  maLocalTime(void))
	{
		SYSLOG("maLocalTime");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maLocalTime", "()I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(int,  maGetMilliSecondCount(void))
	{
//		SYSLOG("maGetMilliSecondCount");
		
		jclass cls = mJNIEnv->GetObjectClass(mJThis);
		jmethodID methodID = mJNIEnv->GetMethodID(cls, "maGetMilliSecondCount", "()I");
		if (methodID == 0) ERROR_EXIT;
		int retval = mJNIEnv->CallIntMethod(mJThis, methodID);
		
		mJNIEnv->DeleteLocalRef(cls);
		
		return retval;
	}

	SYSCALL(int,  maFreeObjectMemory(void))
	{
		SYSLOG("maFreeObjectMemory NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(int,  maTotalObjectMemory(void))
	{
		SYSLOG("maTotalObjectMemory NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(int,  maVibrate(int ms))
	{
		SYSLOG("maVibrate NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(void, maPanic(int result, const char* message))
	{
		SYSLOG("maPanic NOT IMPLEMENTED");
	}

	SYSCALL(int,  maSoundPlay(MAHandle sound_res, int offset, int size))
	{
		SYSLOG("maSoundPlay NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(void,  maSoundStop(void))
	{
		SYSLOG("maStopSound NOT IMPLEMENTED");

	}

	SYSCALL(int,  maSoundIsPlaying(void))
	{
		SYSLOG("maSoundIsPlaying NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(int,  maSoundGetVolume(void))
	{
		SYSLOG("maSoundGetVolume NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(void,  maSoundSetVolume(int vol))
	{
		SYSLOG("maSoundSetVolume NOT IMPLEMENTED");

	}

	SYSCALL(int,  maInvokeExtension(int function, int a, int b, int c))
	{
		SYSLOG("maInvokeExtension NOT IMPLEMENTED");
		return -1;
	}

	SYSCALL(int,  maIOCtl(int function, int a, int b, int c))
	{
		SYSLOG("maIOCtl");
		
		switch(function) {

		case maIOCtl_maCheckInterfaceVersion:
			SYSLOG("maIOCtl_maCheckInterfaceVersion");
			return Base::maCheckInterfaceVersion(a);

		case maIOCtl_maWriteLog:
			SYSLOG("maIOCtl_maWriteLog NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maPlatformRequest:
			SYSLOG("maIOCtl_maPlatformRequest NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maSendTextSMS:
			SYSLOG("maIOCtl_maSendTextSMS NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maGetBatteryCharge:
			SYSLOG("maIOCtl_maGetBatteryCharge NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maKeypadIsLocked:
			SYSLOG("maIOCtl_maKeypadIsLocked NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maLockKeypad:
			SYSLOG("maIOCtl_maLockKeypad NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maUnlockKeypad:
			SYSLOG("maIOCtl_maUnlockKeypad NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maBtStartDeviceDiscovery:
			SYSLOG("maIOCtl_maBtStartDeviceDiscovery NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maBtGetNewDevice:
			SYSLOG("maIOCtl_maBtGetNewDevice NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maBtStartServiceDiscovery:
			SYSLOG("maIOCtl_maBtStartServiceDiscovery NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maBtGetNewService:
			SYSLOG("maIOCtl_maBtGetNewService NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maBtGetNextServiceSize:
			SYSLOG("maIOCtl_maBtGetNextServiceSize NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFrameBufferGetInfo:
			SYSLOG("maIOCtl_maFrameBufferGetInfo NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFrameBufferInit:
			SYSLOG("maIOCtl_maFrameBufferInit NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFrameBufferClose:
			SYSLOG("maIOCtl_maFrameBufferClose NOT IMPLEMENTED");
			return -1;
/*
		case maIOCtl_maAudioBufferInit:
			SYSLOG("maIOCtl_maAudioBufferInit NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maAudioBufferReady:
			SYSLOG("maIOCtl_maAudioBufferReady NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maAudioBufferClose:
			SYSLOG("maIOCtl_maAudioBufferClose NOT IMPLEMENTED");
			return -1;
*/
		case maIOCtl_maLocationStart:
			SYSLOG("maIOCtl_maLocationStart NOT IMPLEMENTED");
			return -1;
			
		case maIOCtl_maLocationStop:
			SYSLOG("maIOCtl_maLocationStop NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileOpen:
			SYSLOG("maIOCtl_maFileOpen NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileExists:
			SYSLOG("maIOCtl_maFileExists NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileClose:
			SYSLOG("maIOCtl_maFileClose NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileCreate:
			SYSLOG("maIOCtl_maFileCreate NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileDelete:
			SYSLOG("maIOCtl_maFileDelete NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileSize:
			SYSLOG("maIOCtl_maFileSize NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileAvailableSpace:
			SYSLOG("maIOCtl_maFileAvailableSpace NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileTotalSpace:
			SYSLOG("maIOCtl_maFileTotalSpace NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileDate:
			SYSLOG("maIOCtl_maFileDate NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileRename:
			SYSLOG("maIOCtl_maFileRename NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileTruncate:
			SYSLOG("maIOCtl_maFileTruncate NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileWrite:
			SYSLOG("maIOCtl_maFileWrite NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileWriteFromData:
			SYSLOG("maIOCtl_maFileWriteFromData NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileRead:
			SYSLOG("maIOCtl_maFileRead NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileReadToData:
			SYSLOG("maIOCtl_maFileReadToData NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileTell:
			SYSLOG("maIOCtl_maFileTell NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileSeek:
			SYSLOG("maIOCtl_maFileSeek NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maFileListStart:
			SYSLOG("maIOCtl_maFileListStart NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileListNext:
			SYSLOG("maIOCtl_maFileListNext NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maFileListClose:
			SYSLOG("maIOCtl_maFileListClose NOT IMPLEMENTED");
			return -1;
/*
		case maIOCtl_maStartVideoStream:
			SYSLOG("maIOCtl_maStartVideoStream NOT IMPLEMENTED");
			return -1;

		case maIOCtl_maPauseStream:
			SYSLOG("maIOCtl_maPauseStream NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maResumeStream:
			SYSLOG("maIOCtl_maResumeStream NOT IMPLEMENTED");
			return -1;
		
		case maIOCtl_maCloseStream:
			SYSLOG("maIOCtl_maCloseStream NOT IMPLEMENTED");
			return -1;
*/			
		}
		
		return IOCTL_UNAVAILABLE;
	}
}

void MoSyncExit(int errorCode)
{
	__android_log_write(ANDROID_LOG_INFO, "MoSyncExit!", "MOSYNC ERROR");
	int yield = Core::GetVMYield(gCore);
	yield = 1;
}

void MoSyncErrorExit(int errorCode)
{
	__android_log_write(ANDROID_LOG_INFO, "MoSyncErrorExit!", "MOSYNC ERROR");
	int yield = Core::GetVMYield(gCore);
	yield = 1;
	
	jclass cls = Base::mJNIEnv->GetObjectClass(Base::mJThis);
	jmethodID methodID = Base::mJNIEnv->GetMethodID(cls, "waitThread", "()V");
	if (methodID == 0) ERROR_EXIT;
	Base::mJNIEnv->CallVoidMethod(Base::mJThis, methodID);
	
	Base::mJNIEnv->DeleteLocalRef(cls);
	
}