// Interface Pixel Streaming Module

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "IInputDeviceModule.h"
#include "Templates/SharedPointer.h"

class UTexture2D;
class IMediaPlayer;
class IMediaEventSink;

/**
* 该模块的公开接口
*/
class IPixelStreamingModule : public IInputDeviceModule
{
public:

	/**
	* 对该模块的接口进行类似于单例的访问。这只是为了方便！
	* 不过，请注意在关闭阶段调用此命令。您的模块可能已经卸载。
	*
	* @return 返回单例，根据需要加载模块
	*/
	static inline IPixelStreamingModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IPixelStreamingModule>("PixelStreamer");
	}

	/**
	* 检查此模块是否已加载并准备就绪。只有当 IsAvailable() 返回 true 时，调用 Get() 才有效。
	*
	* @return True 如果模块已加载并准备使用
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("PixelStreamer");
	}

	/**
	 * 返回对输入设备的引用。此引用的生存期是基础共享指针的生存期。
	 * @return 对输入设备的引用
	 */
	virtual class FInputDevice& GetInputDevice() = 0;
	
	/**
	 * 将任何播放器配置JSON添加到给定对象中，该对象与为浏览器上的像素流配置输入系统有关。
	 * @param JsonObject - 要向其中添加字段的JSON对象。
	 */
	virtual void AddPlayerConfig(TSharedRef<class FJsonObject>& JsonObject) = 0;

	/**
	 * 将数据响应发送回我们发送视频的浏览器。例如，这可以用作对UI交互的响应。
	 * @param Descriptor - 通用描述符字符串。
	 */
	virtual void SendResponse(const FString& Descriptor) = 0;

	/**
	 * 将数据命令发送回我们发送视频的浏览器。这与响应不同，因为命令是低级别的并且来自UE4而不是像素流应用。
	 * @param Descriptor - 通用描述符字符串。
	 */
	virtual void SendCommand(const FString& Descriptor) = 0;

	/**
	 * 冻结像素流。
	 * @param Texture - 要显示的冻结帧。如果为null，则捕获后台缓冲区。
	 */
	virtual void FreezeFrame(UTexture2D* Texture) = 0;

	/**
	 * 解冻像素流。
	 */
	virtual void UnfreezeFrame() = 0;

	// 播放器

	virtual bool IsPlayerInitialized() const = 0;

	virtual TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe> CreatePlayer(IMediaEventSink& EventSink) = 0;
};

