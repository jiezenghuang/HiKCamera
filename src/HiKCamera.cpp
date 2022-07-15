#include "HiKCamera.h"

#include <fal/common/Logger.h>
#include <fal/common/ErrorCategory.h>
#include <fal/plugin/PluginManager.hpp>

using namespace fal;

HiKCameraPlugin::HiKCameraPlugin(const PluginProfile& profile, void* handle)
    : AbstractCameraPlugin(profile), open_(false), exposure_(1000), trigger_(true), handle_(handle)
{
    iprofile_.name = "HiKCameraPlugin";
    iprofile_.vendor = "Wingtech";
    iprofile_.model = "Unkown";
    iprofile_.sn = "Undefine";
    iprofile_.version = "Unkown";
}

HiKCameraPlugin::~HiKCameraPlugin() {}

void HiKCameraPlugin::ParseProfile(const MV_CC_DEVICE_INFO& info, PluginInstanceProfile& profile)
{
	if (info.nTLayerType == MV_GIGE_DEVICE)
	{
		profile.vendor.assign((char*)info.SpecialInfo.stGigEInfo.chManufacturerName);
		profile.model.assign((char*)info.SpecialInfo.stGigEInfo.chModelName);
		profile.sn.assign((char*)info.SpecialInfo.stGigEInfo.chSerialNumber);
		profile.version.assign((char*)info.SpecialInfo.stGigEInfo.chDeviceVersion);
		//profile.name.assign((char*)info.SpecialInfo.stGigEInfo.chUserDefinedName);
	}
	else
	{
		profile.vendor.assign((char*)info.SpecialInfo.stUsb3VInfo.chManufacturerName);
		profile.model.assign((char*)info.SpecialInfo.stUsb3VInfo.chModelName);
		profile.sn.assign((char*)info.SpecialInfo.stUsb3VInfo.chSerialNumber);
		profile.version.assign((char*)info.SpecialInfo.stUsb3VInfo.chDeviceVersion);
		//profile.name.assign((char*)info.SpecialInfo.stUsb3VInfo.chUserDefinedName);
	}
}

int HiKCameraPlugin::open() 
{            
	if (handle_ == nullptr)
		return EC_FAIL_ARGS_INVALID;

	if (this->isOpen())
		return EC_SUCESS;

	int code = MV_CC_OpenDevice(handle_, MV_ACCESS_Exclusive);
	if (code != MV_OK)
	{
        LOG_E("open camera fail, error {:#x}", static_cast<uint32_t>(code));
		return EC_FAIL_DEVICE_NOT_AVAILABLE;
	}

	code = MV_CC_SetEnumValue(handle_, "ExposureMode", MV_EXPOSURE_MODE_TIMED);
	if (code != MV_OK)
	{
        LOG_E("set global exposure mode fail, error {:#x}", static_cast<uint32_t>(code));
		return EC_FAIL_API_ERROR;
	}

	code = MV_CC_SetEnumValue(handle_, "TriggerMode", MV_TRIGGER_MODE_OFF);
	if (code != MV_OK)
	{
        LOG_E("turn off trigger mode fail, error {:#x}", static_cast<uint32_t>(code));
		return EC_FAIL_API_ERROR;
	}

	code = MV_CC_RegisterImageCallBackEx(handle_, ImageCallBack, this);
	if (code != MV_OK)
	{
        LOG_E("set image call back fail, error {:#x}", static_cast<uint32_t>(code));
		return EC_FAIL_API_ERROR;
	}

	MV_IMAGE_BASIC_INFO image_info = { 0 };
	code = MV_CC_GetImageInfo(handle_, &image_info);
	if (MV_OK != code)
	{
        LOG_E("get image size fail, error {:#x}", static_cast<uint32_t>(code));
		return EC_FAIL_API_ERROR;
	}

	property_["Height"] = image_info.nHeightValue;
	property_["Width"] = image_info.nWidthValue;

	MV_CC_DEVICE_INFO device_info = { 0 };
	code = MV_CC_GetDeviceInfo(handle_, &device_info);
	if (code == MV_OK)
	{
		if (device_info.nTLayerType == MV_GIGE_DEVICE)
		{
			int nPacketSize = MV_CC_GetOptimalPacketSize(handle_);
			if (nPacketSize > 0)
			{
				code = MV_CC_SetIntValue(handle_, "GevSCPSPacketSize", nPacketSize);
				if (code != MV_OK)
                    LOG_W("set packet size fail, error {:#x}", static_cast<uint32_t>(code));
			}
		}
		ParseProfile(device_info, iprofile_);
		iprofile_.available = true;
	}

	open_ = true;
	LOG_D("open {} camera {} sucess, serial no {}, resolution {} x {}", iprofile_.vendor,
        iprofile_.model, iprofile_.sn, property_["Width"].get<int>(), property_["Height"].get<int>());

	MVCC_ENUMVALUE pixel_type;
	code = MV_CC_GetEnumValue(handle_, "PixelFormat", &pixel_type);
	if (code == MV_OK && pixel_type.nCurValue != PixelType_Gvsp_Mono8)
	{
		LOG_W("camera using pixel format {:#x}, not Mono8 {:#x}", pixel_type.nCurValue, PixelType_Gvsp_Mono8);
	}

    // if (pixel_type.nCurValue == PixelType_Gvsp_Mono8)
    // {
    //     code = MV_CC_SetEnumValue(handle_, "ADCBitDepth", 2);
    //     if (code != MV_OK)
    //     {
    //         LOG_W("set ADC bit depth fail, error {:#x}", static_cast<uint32_t>(code));
    //         //return HY_FAIL_API_ERROR;
    //     }
    // }

    // code = MV_CC_SetBoolValue(handle_, "AcquisitionFrameRateEnable", false);
    // if (code != MV_OK)
    // {
    //     LOG_W("set frame rate enable fail, error {:#x}", static_cast<uint32_t>(code));
    //     //return HY_FAIL_API_ERROR;
    // }
    return EC_SUCESS;
}

bool HiKCameraPlugin::isOpen() 
{
	if (handle_ == nullptr)
		return false;
	return MV_CC_IsDeviceConnected(handle_);
}

int HiKCameraPlugin::setExposure(int us) 
{
    if (!this->isOpen())
		return EC_FAIL_DEVICE_CLOSED;

	int ec = MV_CC_SetExposureTime(handle_, us);
	if (MV_OK != ec)
	{
        LOG_E("set exposure time fail, error {:#x}", static_cast<uint32_t>(ec));
		return EC_FAIL_API_ERROR;
	}
	property_["Exposure"] = us;
    return EC_SUCESS;
}

int HiKCameraPlugin::getExposure() 
{
	if (!this->isOpen())
		return EC_FAIL_DEVICE_CLOSED;

	MVCC_FLOATVALUE val = { 0 };
	int code = MV_CC_GetExposureTime(handle_, &val);
	if (code != MV_OK)
	{
        LOG_E("get exposure time fail, error {:#x}", static_cast<uint32_t>(code));
		return EC_FAIL_API_ERROR;
	}
	property_["Exposure"] = val.fCurValue;
	return property_["Exposure"];
}

int HiKCameraPlugin::setTriggerMode(bool on) 
{
    int ec = MV_CC_SetEnumValue(handle_, "TriggerMode", (on ? MV_TRIGGER_MODE_ON : MV_TRIGGER_MODE_OFF));
	if (ec != MV_OK)
	{
		LOG_E("turn {} trigger mode fail, error {:#x}", (on ? "on" : "off"), static_cast<uint32_t>(ec));
		return EC_FAIL_API_ERROR;
	}
    property_["TriggerMode"] = on;
	LOG_D("trigger mode set to {}", on);
    return EC_SUCESS;
}

bool HiKCameraPlugin::getTriggerMode() 
{
	MVCC_ENUMVALUE trigger_mode;
	int ec = MV_CC_GetEnumValue(handle_, "TriggerMode", &trigger_mode);
	if (ec != MV_OK)
	{
		LOG_E("get trigger mode fail, error {:#x}", static_cast<uint32_t>(ec));
		return EC_FAIL_API_ERROR;
	}
	property_["TriggerMode"] = trigger_mode.nCurValue == MV_TRIGGER_MODE_ON;
    return property_["TriggerMode"];
}

int HiKCameraPlugin::setTriggerSource(const std::string& source) 
{
    int ec = EC_SUCESS;
    if(source == "Software")
        ec = MV_CC_SetEnumValue(handle_, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    else if(source == "Line0")
        ec = MV_CC_SetEnumValue(handle_, "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
    else if(source == "Line1")
        ec = MV_CC_SetEnumValue(handle_, "TriggerSource", MV_TRIGGER_SOURCE_LINE1);
    else if(source == "Line2")
        ec = MV_CC_SetEnumValue(handle_, "TriggerSource", MV_TRIGGER_SOURCE_LINE2);
    else if(source == "Line3")
        ec = MV_CC_SetEnumValue(handle_, "TriggerSource", MV_TRIGGER_SOURCE_LINE3);
    else 
    {
        LOG_E("invalid triger source {}", source);
        return EC_FAIL_ARGS_INVALID;
    }
    if (ec != MV_OK)
    {
        LOG_E("set trigger source to {} fail, error {:#x}", source, static_cast<uint32_t>(ec));
        return EC_FAIL_API_ERROR;
    }
    property_["TriggerSource"] = source;
	LOG_D("trigger source set to {}", source);
    return EC_SUCESS;
}

std::string HiKCameraPlugin::getTriggerSource() 
{
	property_["TriggerMode"] = "Unknown";
	MVCC_ENUMVALUE trigger_source;
	int ec = MV_CC_GetEnumValue(handle_, "TriggerSource", &trigger_source);
	if (ec != MV_OK)
	{
		LOG_E("get trigger source fail, error {:#x}", static_cast<uint32_t>(ec));
		return property_["TriggerMode"];
	}

	switch (trigger_source.nCurValue)
	{
	case MV_TRIGGER_SOURCE_SOFTWARE:
		property_["TriggerSource"] = "Software";
		break;
	case MV_TRIGGER_SOURCE_LINE0:
		property_["TriggerSource"] = "Line0";
		break;
	case MV_TRIGGER_SOURCE_LINE1:
		property_["TriggerSource"] = "Line1";
		break;
	case MV_TRIGGER_SOURCE_LINE2:
		property_["TriggerSource"] = "Line2";
		break;
	case MV_TRIGGER_SOURCE_LINE3:
		property_["TriggerSource"] = "Line3";
		break;
	default:
		break;
	}
    return property_["TriggerSource"];
}

int HiKCameraPlugin::trigger() 
{
	if (!this->isOpen())
		return EC_FAIL_DEVICE_CLOSED;

	int ec = MV_CC_TriggerSoftwareExecute(handle_);
	if (ec != MV_OK)
	{
        LOG_E("soft trigger fail, error {:#x}", static_cast<uint32_t>(ec));
		return EC_FAIL_API_ERROR;
	}
	return EC_SUCESS;
}

int HiKCameraPlugin::start()
{
	int code = MV_CC_StartGrabbing(handle_);
	if (code != MV_OK)
    {
        LOG_E("start grabbing fail, error {:#x}", static_cast<uint32_t>(code));
        return EC_FAIL_API_ERROR;
    }
	return EC_SUCESS;
}

int HiKCameraPlugin::stop()
{
	int code = MV_CC_StopGrabbing(handle_);
	if (code != MV_OK)
    {
        LOG_E("stop grabbing fail, error {:#x}", static_cast<uint32_t>(code));
        return EC_FAIL_API_ERROR;
    }
	return EC_SUCESS;
}

void HiKCameraPlugin::close() 
{
    open_ = false;
    LOG_D("{} plugin {} close", profile_.name, iprofile_.name);
}

void __stdcall HiKCameraPlugin::ImageCallBack(unsigned char * pImage, MV_FRAME_OUT_INFO_EX * pFrameInfo, void * pUser)
{
	HiKCameraPlugin* instance = static_cast<HiKCameraPlugin*>(pUser);
	if (pFrameInfo && instance->capture_callback_ != nullptr)
	{
		cv::Mat img;
		MvGvspPixelType enDstPixelType = PixelType_Gvsp_Undefined;
		int channel = 1;
		bool convert = false;
		switch (pFrameInfo->enPixelType)
		{
		case PixelType_Gvsp_BGR8_Packed:
		case PixelType_Gvsp_YUV422_Packed:
		case PixelType_Gvsp_YUV422_YUYV_Packed:
		case PixelType_Gvsp_BayerGR8:
		case PixelType_Gvsp_BayerRG8:
		case PixelType_Gvsp_BayerGB8:
		case PixelType_Gvsp_BayerBG8:
		case PixelType_Gvsp_BayerGB10:
		case PixelType_Gvsp_BayerGB10_Packed:
		case PixelType_Gvsp_BayerBG10:
		case PixelType_Gvsp_BayerBG10_Packed:
		case PixelType_Gvsp_BayerRG10:
		case PixelType_Gvsp_BayerRG10_Packed:
		case PixelType_Gvsp_BayerGR10:
		case PixelType_Gvsp_BayerGR10_Packed:
		case PixelType_Gvsp_BayerGB12:
		case PixelType_Gvsp_BayerGB12_Packed:
		case PixelType_Gvsp_BayerBG12:
		case PixelType_Gvsp_BayerBG12_Packed:
		case PixelType_Gvsp_BayerRG12:
		case PixelType_Gvsp_BayerRG12_Packed:
		case PixelType_Gvsp_BayerGR12:
		case PixelType_Gvsp_BayerGR12_Packed:
			enDstPixelType = PixelType_Gvsp_RGB8_Packed;
			channel = 3;
			convert = true;
			break;
		case PixelType_Gvsp_Mono10:
		case PixelType_Gvsp_Mono10_Packed:
		case PixelType_Gvsp_Mono12:
		case PixelType_Gvsp_Mono12_Packed:
			enDstPixelType = PixelType_Gvsp_Mono8;
			channel = 1;
			convert = true;
			break;
		case PixelType_Gvsp_Mono8:
			enDstPixelType = PixelType_Gvsp_Mono8;
			channel = 1;
			break;
		default:
			break;
		}

		if(enDstPixelType == PixelType_Gvsp_Undefined)
		{
			LOG_E("undefine pixel type {:#x}", pFrameInfo->enPixelType);
			return;
		}

		if(convert)
		{
			if(instance->convert_buffer_ == nullptr || 
				instance->convert_buffer_size_ != pFrameInfo->nWidth * pFrameInfo->nHeight * channel)
			{
				if (instance->convert_buffer_ != nullptr)
					delete[] instance->convert_buffer_;
				
				instance->convert_buffer_size_ = pFrameInfo->nWidth * pFrameInfo->nHeight * channel;
				if (instance->convert_buffer_size_ % 16 != 0)
				{
					LOG_E("invalid buffer size {} x {}", pFrameInfo->nWidth, pFrameInfo->nHeight);
					return;
				}
				instance->convert_buffer_ = new uint8_t[instance->convert_buffer_size_];
				if (instance->convert_buffer_ == nullptr)
				{
					LOG_E("malloc convert buffer fail");
					return;
				}
			}

			MV_CC_PIXEL_CONVERT_PARAM param = { 0 };
			memset(&param, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));
			param.nWidth = pFrameInfo->nWidth; 
			param.nHeight = pFrameInfo->nHeight;  
			param.pSrcData = pImage;      
			param.nSrcDataLen = pFrameInfo->nFrameLen;  
			param.enSrcPixelType = pFrameInfo->enPixelType;  
			param.enDstPixelType = enDstPixelType;
			param.pDstBuffer = instance->convert_buffer_;
			param.nDstBufferSize = instance->convert_buffer_size_;
			int code = MV_CC_ConvertPixelType(instance->handle_, &param);
			if (code != MV_OK)
			{
                LOG_E("convert pixel type fail, error {:#x}", static_cast<uint32_t>(code));
				return;
			}
			img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, 
				channel == 1 ? CV_8UC1 : CV_8UC3, instance->convert_buffer_);
		}
		else
		{
			img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pImage);
		}

		// if (pFrameInfo->enPixelType != PixelType_Gvsp_Mono8)
		// {
		// 	if (instance->convert_buffer_ == nullptr)
		// 	{
		// 		instance->convert_buffer_size_ = pFrameInfo->nWidth * pFrameInfo->nHeight;
		// 		if (instance->convert_buffer_size_ % 16 != 0)
		// 		{
		// 			LOG_E("invalid buffer size {} x {}", pFrameInfo->nWidth, pFrameInfo->nHeight);
		// 			return;
		// 		}
		// 		instance->convert_buffer_ = new uint8_t[instance->convert_buffer_size_];
		// 		if (instance->convert_buffer_ == nullptr)
		// 		{
		// 			LOG_E("malloc convert buffer fail");
		// 			return;
		// 		}
		// 		//HY_LOG_SEV(debug) << "convert buffer malloc, size " << instance->convert_buffer_size_;
		// 	}

		// 	MV_CC_PIXEL_CONVERT_PARAM param = { 0 };
		// 	memset(&param, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));
		// 	param.nWidth = pFrameInfo->nWidth; 
		// 	param.nHeight = pFrameInfo->nHeight;  
		// 	param.pSrcData = pImage;      
		// 	param.nSrcDataLen = pFrameInfo->nFrameLen;  
		// 	param.enSrcPixelType = pFrameInfo->enPixelType;  
		// 	param.enDstPixelType = PixelType_Gvsp_Mono8;
		// 	param.pDstBuffer = instance->convert_buffer_;
		// 	param.nDstBufferSize = instance->convert_buffer_size_;
		// 	int code = MV_CC_ConvertPixelType(instance->handle_, &param);
		// 	if (code != MV_OK)
		// 	{
        //         LOG_E("convert pixel type fail, error {:#x}", static_cast<uint32_t>(code));
		// 		return;
		// 	}
		// 	img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, instance->convert_buffer_);
		// }
		// else
		// {
		// 	img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pImage);
		// }
				
		try
		{
			instance->capture_callback_(instance, img.clone());
		}
		catch (const std::exception& ex)
		{
			LOG_E("on capture image catch exception: ", ex.what());
		}
	}
}