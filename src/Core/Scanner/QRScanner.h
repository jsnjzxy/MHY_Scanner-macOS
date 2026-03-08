#pragma once

#include <string>
#include <memory>

#include <opencv2/opencv.hpp>

class QRScanner
{
public:
	QRScanner();
	~QRScanner();
	void decodeSingle(const cv::Mat& img, std::string& qrCode);
	void decodeMultiple(const cv::Mat& img, std::string& qrCode);

#ifdef __APPLE__
	// 优化版本：直接从 BGRA 缓冲区识别，跳过 cv::Mat 转换
	void decodeFromBGRA(const uint8_t* bgraData, int width, int height, int stride, std::string& qrCode);

	// GPU 直连版本：直接从 CVPixelBuffer 识别，零拷贝（用于硬件解码）
	void decodeFromCVPixelBuffer(void* pixelBuffer, int width, int height, std::string& qrCode);
#endif

private:
#ifdef __APPLE__
	// Vision 框架实现 (通过 PIMPL 模式隐藏 Objective-C)
	class Impl;
	std::unique_ptr<Impl> pImpl;
#else
	void* detector;  // WeChatQRCode (避免引入头文件)
#endif

	bool useVision{ false };
};
