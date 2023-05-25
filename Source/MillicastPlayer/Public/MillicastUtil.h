#pragma once

#include "Async/Async.h"

template<typename TCallback>
void AsyncGameThreadTaskUnguarded(TCallback Callback)
{
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		Callback();
	});
}

template<typename TCapture, typename TCallback>
void AsyncGameThreadTaskWithCapture(TWeakObjectPtr<TCapture> WeakCapture, TCallback Callback)
{
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		if (!WeakCapture.IsValid())
		{
			return;
		}
		
		Callback();
	});
}

template<typename TCapture, typename TCallback>
void AsyncGameThreadTask(TCapture* CaptureObject, TCallback Callback)
{
	TWeakObjectPtr<TCapture> WeakCapture(CaptureObject);
	AsyncGameThreadTaskWithCapture(WeakCapture, Callback);
}
