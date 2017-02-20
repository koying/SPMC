#pragma once

/*
*      Copyright (C) 2016 liberty-developer
*      https://github.com/liberty-developer
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  <http://www.gnu.org/licenses/>.
*
*/

#include "ap4/Ap4.h"
#include "../SSD_dll.h"
#include "media/NdkMediaDrm.h"

#include <string>

static const uint8_t guid_widevine[] = { 0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6, 0x4a, 0xce, 0xa3, 0xc8, 0x27, 0xdc, 0xd5, 0x1d, 0x21, 0xed };
static const uint8_t guid_playready[] = { 0x9A, 0x04, 0xF0, 0x79, 0x98, 0x40, 0x42, 0x86, 0xAB, 0x92, 0xE6, 0x5B, 0xE0, 0x88, 0x5F, 0x95 };

/*----------------------------------------------------------------------
|   WV_CencSingleSampleDecrypter
+---------------------------------------------------------------------*/
class WV_CencSingleSampleDecrypter_android : public AP4_CencSingleSampleDecrypter
{
public:
  // methods
  WV_CencSingleSampleDecrypter_android(std::string licenseURL, uint8_t* guid, AP4_DataBuffer &pssh, AP4_DataBuffer &serverCertificate);
  ~WV_CencSingleSampleDecrypter_android();

  bool initialized()const { return media_drm_ != 0; }

  virtual AP4_Result SetFrameInfo(const AP4_UI16 key_size, const AP4_UI08 *key, const AP4_UI08 nal_length_size);

  virtual AP4_Result DecryptSampleData(AP4_DataBuffer& data_in,
    AP4_DataBuffer& data_out,

    // always 16 bytes
    const AP4_UI08* iv,

    // pass 0 for full decryption
    unsigned int    subsample_count,

    // array of <subsample_count> integers. NULL if subsample_count is 0
    const AP4_UI16* bytes_of_cleartext_data,

    // array of <subsample_count> integers. NULL if subsample_count is 0
    const AP4_UI32* bytes_of_encrypted_data);

private:
  bool ProvisionRequest();
  bool GetLicense();
  bool SendSessionMessage();

  AMediaDrm *media_drm_;
  AMediaDrmByteArray session_id_;
  const uint8_t *key_request_;
  size_t key_request_size_;

  std::string pssh_, license_url_;
  AP4_UI16 key_size_;
  uint8_t key_[32];
  AP4_UI08 nal_length_size_;
};

class WVDecrypter : public SSD::SSD_DECRYPTER
{
public:
  // Return supported URN if type matches to capabikitues, otherwise null
  virtual const char *Supported(const char* licenseType, const char *licenseKey) override
  {
    licenseKey_ = licenseKey;
    if (strcmp(licenseType, "com.widevine.alpha") == 0)
    {
      memcpy(guid_, guid_widevine, 16);
      return "urn:uuid:EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED";
    }
    else if (strcmp(licenseType, "com.microsoft.playready") == 0)
    {
      memcpy(guid_, guid_playready, 16);
      return "urn:uuid:9a04f079-9840-4286-ab92e65be0885f95";
    }
    return 0;
  }

  virtual AP4_CencSingleSampleDecrypter *CreateSingleSampleDecrypter(AP4_DataBuffer &streamCodec, AP4_DataBuffer &serverCertificate) override
  {
    AP4_CencSingleSampleDecrypter *res = new WV_CencSingleSampleDecrypter_android(licenseKey_, guid_, streamCodec, serverCertificate);
    if (!((WV_CencSingleSampleDecrypter_android*)res)->initialized())
    {
      delete res;
      res = 0;
    }
    return res;
  }
  
  virtual bool OpenVideoDecoder(const SSD::SSD_VIDEOINITDATA *initData)
  {
    return false;
  }

  virtual SSD::SSD_DECODE_RETVAL DecodeVideo(SSD::SSD_SAMPLE *sample, SSD::SSD_PICTURE *picture)
  {
    return SSD::VC_ERROR;
  }
private:
  std::string licenseKey_;
  uint8_t guid_[16];
};

