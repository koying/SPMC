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

#include "wvdecrypter_android.h"

#include "../helpers.h"
#include "jsmn.h"
#include <stdarg.h>
#include <deque>
#include <chrono>
#include <thread>

#include "filesystem/File.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "utils/log.h"

using namespace SSD;

/*******************************************************
CDM
********************************************************/

bool needProvision = false;

void MediaDrmEventListener(AMediaDrm *media_drm, const AMediaDrmSessionId *sessionId, AMediaDrmEventType eventType, int extra, const uint8_t *data, size_t dataSize)
{
  CLog::Log(LOGDEBUG, "EVENT occured drm:%x, event:%d extra:%d dataSize;%d", (unsigned int)media_drm, eventType, extra, dataSize);
  if (eventType == EVENT_PROVISION_REQUIRED )
    needProvision = true;

}

/*----------------------------------------------------------------------
|   WV_CencSingleSampleDecrypter::WV_CencSingleSampleDecrypter
+---------------------------------------------------------------------*/

WV_CencSingleSampleDecrypter_android::WV_CencSingleSampleDecrypter_android(std::string licenseType, std::string licenseURL, AP4_DataBuffer &init_data, AP4_DataBuffer &serverCertificate)
  : AP4_CencSingleSampleDecrypter(0)
  , media_drm_(0)
  , license_type_(licenseType)
  , license_url_(licenseURL)
  , pssh_(std::string(reinterpret_cast<const char*>(init_data.GetData()), init_data.GetDataSize()))
  , key_size_(0)
  , nal_length_size_(0)
{
//  SetParentIsOwner(false);

#ifdef _DEBUG
  std::string strDbg = "special://temp/EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED.init";
  XFILE::CFile f;
  f.OpenForWrite(strDbg, true);
  f.Write(pssh_.c_str(), pssh_.size());
  f.Close();
#endif
  static const uint8_t guid_widevine[] = { 0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6, 0x4a, 0xce, 0xa3, 0xc8, 0x27, 0xdc, 0xd5, 0x1d, 0x21, 0xed };
  static const uint8_t guid_playready[] = { 0x9A, 0x04, 0xF0, 0x79, 0x98, 0x40, 0x42, 0x86, 0xAB, 0x92, 0xE6, 0x5B, 0xE0, 0x88, 0x5F, 0x95 };
  
  const uint8_t* guid;
  if (license_type_== "com.widevine.alpha")
    guid = guid_widevine;
  else if (license_type_== "com.microsoft.playready")
    guid = guid_playready;
  else
  {
    CLog::Log(LOGERROR, "Unsupported crypto scheme ! : %s", license_type_.c_str());
    return;
  }
  
  if (strcmp(&pssh_[4], "pssh") != 0)
  {
    static const uint8_t pssh_header[] = { 0x70, 0x73, 0x73, 0x68, 0x00, 0x00, 0x00, 0x00 };

    uint32_t boxLen = 0;
    uint32_t dataLen = pssh_.size();

    pssh_.insert(0, reinterpret_cast<const char*>(pssh_header), sizeof(pssh_header));
    boxLen += sizeof(pssh_header);
    pssh_.insert(boxLen, reinterpret_cast<const char*>(guid), 16);
    boxLen += 16;

    pssh_.insert(boxLen, reinterpret_cast<const char*>(&dataLen), sizeof(uint32_t));
    boxLen += sizeof(uint32_t);

    boxLen += sizeof(uint32_t) /* boxlen */ + dataLen;
    pssh_.insert(0, reinterpret_cast<const char*>(&boxLen), sizeof(uint32_t));
  }

#ifdef _DEBUG
  strDbg = "special://temp/EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED.init2";
  f.OpenForWrite(strDbg, true);
  f.Write(pssh_.c_str(), pssh_.size());
  f.Close();
#endif

  //Build up a CDM path to store decrypter specific stuff. Each domain gets it own path
  /*
  const char* bspos(strchr(license_url_.c_str(), ':'));
  if (!bspos || bspos[1] != '/' || bspos[2] != '/' || !(bspos = strchr(bspos + 3, '/')))
  {
    CLog::Log(LOGERROR, "Could not find protocol inside url - invalid");
    return;
  }
  if (bspos - license_url_.c_str() > 256)
  {
    CLog::Log(LOGERROR, "Length of domain exeeds max. size of 256 - invalid");
    return;
  }
  char buffer[1024];
  buffer[(bspos - license_url_.c_str()) * 2] = 0;
  AP4_FormatHex(reinterpret_cast<const uint8_t*>(license_url_.c_str()), bspos - license_url_.c_str(), buffer);
  */

  if (!AMediaDrm_isCryptoSchemeSupported(guid, "video/mp4"))
  {
    CLog::Log(LOGERROR, "Crypto scheme not supported by device ! : %s", license_type_.c_str());
    return;
  }
  
  media_drm_ = AMediaDrm_createByUUID(guid);
  if (!media_drm_)
  {
    CLog::Log(LOGERROR, "Unable to initialize media_drm");
    return;
  }
  CLog::Log(LOGDEBUG, "Successful instanciated media_drm: %X", (unsigned int)media_drm_);

  media_status_t status;
  if ((status = AMediaDrm_setOnEventListener(media_drm_, MediaDrmEventListener)) != AMEDIA_OK)
  {
    CLog::Log(LOGERROR, "Unable to install Event Listener (%d)", status);
    AMediaDrm_release(media_drm_);
    media_drm_ = 0;
    return;
  }

  memset(&session_id_, 0, sizeof(session_id_));
  if ((status = AMediaDrm_openSession(media_drm_, &session_id_)) != AMEDIA_OK)
  {
    CLog::Log(LOGERROR, "Unable to open DRM session (%d)", status);
    AMediaDrm_release(media_drm_);
    media_drm_ = 0;
    return;
  }

TRYAGAIN:
  if (!GetLicense())
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (needProvision && ProvisionRequest())
    {
      needProvision = false;
      goto TRYAGAIN;
    }
    CLog::Log(LOGERROR, "Unable to generate a license");
    AMediaDrm_closeSession(media_drm_, &session_id_);
    AMediaDrm_release(media_drm_);
    media_drm_ = 0;
    return;
  }
}

WV_CencSingleSampleDecrypter_android::~WV_CencSingleSampleDecrypter_android()
{
  if (media_drm_)
  {
    AMediaDrm_closeSession(media_drm_, &session_id_);
    AMediaDrm_release(media_drm_);
    media_drm_ = 0;
  }
}

bool WV_CencSingleSampleDecrypter_android::ProvisionRequest()
{
  const char *url(0);
  size_t prov_size(4096);

  CLog::Log(LOGERROR, "PrivisionData request: drm: %x key_request_size_: %u", (unsigned int)media_drm_, key_request_size_);

  media_status_t status = AMediaDrm_getProvisionRequest(media_drm_, &key_request_, &prov_size, &url);

  if (status != AMEDIA_OK || !url)
  {
    CLog::Log(LOGERROR, "PrivisionData request failed with status: %d", status);
    return false;
  }
  CLog::Log(LOGDEBUG, "PrivisionData: status: %d, size: %u, url: %s", status, prov_size, url);

  std::string tmp_str("{\"signedRequest\":\"");
  tmp_str += std::string(reinterpret_cast<const char*>(key_request_), prov_size);
  tmp_str += "\"}";

  std::string encoded = b64_encode(reinterpret_cast<const unsigned char*>(tmp_str.data()), tmp_str.size(), false);

  CURL uUrl(url);
  uUrl.SetProtocolOption("Content-Type", "application/json");
  uUrl.SetProtocolOption("seekable", "0");
  uUrl.SetProtocolOption("postdata", encoded.c_str());

  XFILE::CFile* file = new XFILE::CFile();
  if (!file->Open(uUrl, READ_NO_CACHE))
  {
    CLog::Log(LOGERROR, "Provisioning server returned failure");
    return false;
  }

  tmp_str.clear();
  char buf[8192];
  size_t nbRead;

  // read the file
  while ((nbRead = file->Read(buf, 8192)) > 0)
    tmp_str += std::string((const char*)buf, nbRead);
  file->Close();
  delete file;

  status = AMediaDrm_provideProvisionResponse(media_drm_, reinterpret_cast<const uint8_t *>(tmp_str.c_str()), tmp_str.size());

  CLog::Log(LOGDEBUG, "provideProvisionResponse: status %d", status);
  return status == AMEDIA_OK;;
}


bool WV_CencSingleSampleDecrypter_android::GetLicense()
{
  media_status_t status = AMediaDrm_getKeyRequest(media_drm_, &session_id_,
    reinterpret_cast<const uint8_t*>(pssh_.data()), pssh_.size(), "video/mp4", KEY_TYPE_STREAMING,
    0, 0,
    &key_request_, &key_request_size_);

  if (status != AMEDIA_OK || !key_request_size_)
  {
    CLog::Log(LOGERROR, "Key request not successful (%d)", status);
    return false;
  }

  CLog::Log(LOGDEBUG, "Key request successful, size: %u", reinterpret_cast<unsigned int>(key_request_size_));

  if (!SendSessionMessage())
    return false;

  CLog::Log(LOGDEBUG, "License update successful");

  return true;
}

bool WV_CencSingleSampleDecrypter_android::SendSessionMessage()
{
  std::vector<std::string> headers, header, blocks = split(license_url_, '|');
  if (blocks.size() != 4)
  {
    CLog::Log(LOGERROR, "4 '|' separated blocks in licURL expected (req / header / body / response)");
    return false;
  }

#ifdef _DEBUG
  std::string strDbg = "special://temp/EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED.challenge";
  XFILE::CFile f;
  f.OpenForWrite(strDbg, true);
  f.Write(key_request_, key_request_size_);
  f.Close();
#endif

  //Process placeholder in GET String
  std::string::size_type insPos(blocks[0].find("{SSM}"));
  if (insPos != std::string::npos)
  {
    if (insPos >= 0 && blocks[0][insPos - 1] == 'B')
    {
      std::string msgEncoded = b64_encode(key_request_, key_request_size_, true);
      blocks[0].replace(insPos - 1, 6, msgEncoded);
    }
    else
    {
      CLog::Log(LOGERROR, "Unsupported License request template (cmd)");
      return false;
    }
  }

  CURL uUrl(blocks[0].c_str());
  uUrl.SetProtocolOption("acceptencoding", "gzip, deflate");
  uUrl.SetProtocolOption("seekable", "0");
  uUrl.SetProtocolOption("Expect", "");
  
  if (license_type_ == "com.microsoft.playready")
  {
    uUrl.SetProtocolOption("Content-Type", "text/xml");
    uUrl.SetProtocolOption("SOAPAction", "http://schemas.microsoft.com/DRM/2007/03/protocols/AcquireLicense");
  }

  XFILE::CFile* file = nullptr;
  size_t nbRead;
  std::string response;
  char buf[2048];
  AMediaDrmKeySetId dummy_ksid; //STREAMING returns 0
  media_status_t status;

  //Process headers
  headers = split(blocks[1], '&');
  for (std::vector<std::string>::iterator b(headers.begin()), e(headers.end()); b != e; ++b)
  {
    header = split(*b, '=');
    uUrl.SetProtocolOption(trim(header[0]).c_str(), header.size() > 1 ? url_decode(trim(header[1])).c_str() : "");
  }

  //Process body
  if (!blocks[2].empty())
  {
    insPos = blocks[2].find("{SSM}");
    if (insPos != std::string::npos)
    {
      std::string::size_type sidSearchPos(insPos);
      if (insPos >= 0)
      {
        if (blocks[2][insPos - 1] == 'B' || blocks[2][insPos - 1] == 'b')
        {
          std::string msgEncoded = b64_encode(key_request_, key_request_size_, blocks[2][insPos - 1] == 'B');
          blocks[2].replace(insPos - 1, 6, msgEncoded);
        }
        else
          blocks[2].replace(insPos - 1, 6, reinterpret_cast<const char*>(key_request_), key_request_size_);
      }
      else
      {
        CLog::Log(LOGERROR, "Unsupported License request template (body)");
        goto SSMFAIL;
      }

      insPos = blocks[2].find("{SID}", sidSearchPos);
      if (insPos != std::string::npos)
      {
        if (insPos >= 0)
        {
          if (blocks[2][insPos - 1] == 'B' || blocks[2][insPos - 1] == 'b')
          {
            std::string msgEncoded = b64_encode(session_id_.ptr, session_id_.length, blocks[2][insPos - 1] == 'B');
            blocks[2].replace(insPos - 1, 6, msgEncoded);
          }
          else
            blocks[2].replace(insPos - 1, 6, reinterpret_cast<const char*>(session_id_.ptr), session_id_.length);
        }
        else
        {
          CLog::Log(LOGERROR, "Unsupported License request template (body)");
          goto SSMFAIL;
        }
      }
    }
    std::string decoded = b64_encode(reinterpret_cast<const unsigned char*>(blocks[2].data()), blocks[2].size(), false);
    uUrl.SetProtocolOption("postdata", decoded.c_str());
  }

  file = new XFILE::CFile();
  if (!file->Open(uUrl, READ_NO_CACHE))
  {
    CLog::Log(LOGERROR, "License server returned failure");
    goto SSMFAIL;
  }

  // read the file
  while ((nbRead = file->Read(buf, 1024)) > 0)
    response += std::string((const char*)buf, nbRead);
  file->Close();
  delete file;

  if (nbRead != 0)
  {
    CLog::Log(LOGERROR, "Could not read full SessionMessage response");
    goto SSMFAIL;
  }

#ifdef _DEBUG
  strDbg = "special://temp/EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED.response";
  f.OpenForWrite(strDbg, true);
  f.Write(response.c_str(), response.size());
  f.Close();
#endif

  if (!blocks[3].empty())
  {
    if (blocks[3][0] == 'J')
    {
      jsmn_parser jsn;
      jsmntok_t tokens[100];

      jsmn_init(&jsn);
      int i(0), numTokens = jsmn_parse(&jsn, response.c_str(), response.size(), tokens, 100);

      for (; i < numTokens; ++i)
        if (tokens[i].type == JSMN_STRING && tokens[i].size == 1
          && strncmp(response.c_str() + tokens[i].start, blocks[3].c_str() + 2, tokens[i].end - tokens[i].start) == 0)
          break;

      if (i < numTokens)
      {
        if (blocks[3][1] == 'B')
        {
          unsigned int decoded_size = 2048;
          uint8_t decoded[2048];

          b64_decode(response.c_str() + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start, decoded, decoded_size);
          status = AMediaDrm_provideKeyResponse(media_drm_, &session_id_, decoded, decoded_size, &dummy_ksid);
        }
        else
          status = AMediaDrm_provideKeyResponse(media_drm_, &session_id_, reinterpret_cast<const uint8_t*>(response.c_str() + tokens[i + 1].start), tokens[i + 1].end - tokens[i + 1].start, &dummy_ksid);
      }
      else
      {
        CLog::Log(LOGERROR, "Unable to find %s in JSON string", blocks[3].c_str() + 2);
        goto SSMFAIL;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "Unsupported License request template (response)");
      goto SSMFAIL;
    }
  }
  else //its binary - simply push the returned data as update
    status = AMediaDrm_provideKeyResponse(media_drm_, &session_id_, reinterpret_cast<const uint8_t*>(response.data()), response.size(), &dummy_ksid);

  return status == AMEDIA_OK;
SSMFAIL:
  if (file)
    file->Close();
  return false;
}

/*----------------------------------------------------------------------
|   WV_CencSingleSampleDecrypter::SetKeyId
+---------------------------------------------------------------------*/

AP4_Result WV_CencSingleSampleDecrypter_android::SetFrameInfo(const AP4_UI16 key_size, const AP4_UI08 *key, const AP4_UI08 nal_length_size)
{
  if (key_size > 32)
    return AP4_ERROR_INVALID_PARAMETERS;

  key_size_ = key_size;
  memcpy(key_, key, key_size);
  nal_length_size_ = nal_length_size;
  return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   WV_CencSingleSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result WV_CencSingleSampleDecrypter_android::DecryptSampleData(
  AP4_DataBuffer& data_in,
  AP4_DataBuffer& data_out,
  const AP4_UI08* iv,
  unsigned int    subsample_count,
  const AP4_UI16* bytes_of_cleartext_data,
  const AP4_UI32* bytes_of_encrypted_data)
{
  if (!media_drm_)
    return AP4_ERROR_INVALID_STATE;

  if (data_in.GetDataSize() == 0)
  {
    data_out.SetData(reinterpret_cast<const AP4_Byte*>("CRYPTO"), 6);
    uint16_t cryptosize = 6 + 2 + (1 + session_id_.length) + 16;
    data_out.AppendData(reinterpret_cast<const AP4_Byte*>(&cryptosize), sizeof(cryptosize));
    uint8_t dummy(session_id_.length);
    data_out.AppendData(&dummy, 1);
    data_out.AppendData(session_id_.ptr, session_id_.length);
    uint8_t keysystem[16] = { 0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6, 0x4a, 0xce, 0xa3, 0xc8, 0x27, 0xdc, 0xd5, 0x1d, 0x21, 0xed };
    data_out.AppendData(keysystem, 16);
  }
  else
  {
    if (nal_length_size_ > 4)
    {
      CLog::Log(LOGERROR, "Nalu length size > 4 not supported");
      return AP4_ERROR_NOT_SUPPORTED;
    }

    AP4_UI16 dummyClear(0);
    AP4_UI32 dummyCipher(data_in.GetDataSize());
    if (!subsample_count)
    {
      subsample_count = 1;
      bytes_of_cleartext_data = &dummyClear;
      bytes_of_encrypted_data = &dummyCipher;
    }

    data_out.SetData(reinterpret_cast<const AP4_Byte*>(&subsample_count), sizeof(subsample_count));
    data_out.AppendData(reinterpret_cast<const AP4_Byte*>(bytes_of_cleartext_data), subsample_count * sizeof(AP4_UI16));
    data_out.AppendData(reinterpret_cast<const AP4_Byte*>(bytes_of_encrypted_data), subsample_count * sizeof(AP4_UI32));
    data_out.AppendData(reinterpret_cast<const AP4_Byte*>(iv), 16);
    data_out.AppendData(reinterpret_cast<const AP4_Byte*>(key_), 16);

    if (nal_length_size_ && bytes_of_cleartext_data[0] > 0)
    {
      //Note that we assume that there is enough data in data_out to hold everything without reallocating.

      //check NAL / subsample
      const AP4_Byte *packet_in(data_in.GetData()), *packet_in_e(data_in.GetData() + data_in.GetDataSize());
      AP4_Byte *packet_out(data_out.UseData() + data_out.GetDataSize());
      AP4_UI16 *clrb_out(reinterpret_cast<AP4_UI16*>(data_out.UseData() + sizeof(subsample_count)));
      unsigned int nalunitcount(0), nalunitsum(0);

      while (packet_in < packet_in_e && subsample_count)
      {
        uint32_t nalsize(0);
        for (unsigned int i(0); i < nal_length_size_; ++i) { nalsize = (nalsize << 8) + *packet_in++; };

        //Anex-B Start pos
        packet_out[0] = packet_out[1] = packet_out[2] = 0; packet_out[3] = 1;
        packet_out += 4;
        memcpy(packet_out, packet_in, nalsize);
        packet_in += nalsize;
        packet_out += nalsize;
        *clrb_out += (4 - nal_length_size_);
        ++nalunitcount;

        if (nalsize + nal_length_size_ + nalunitsum > *bytes_of_cleartext_data + *bytes_of_encrypted_data)
        {
          CLog::Log(LOGERROR, "NAL Unit exceeds subsample definition (nls: %d) %d -> %d ", nal_length_size_, nalsize + nal_length_size_ + nalunitsum, *bytes_of_cleartext_data + *bytes_of_encrypted_data);
          return AP4_ERROR_NOT_SUPPORTED;
        }
        else if (nalsize + nal_length_size_ + nalunitsum == *bytes_of_cleartext_data + *bytes_of_encrypted_data)
        {
          ++bytes_of_cleartext_data;
          ++bytes_of_encrypted_data;
          ++clrb_out;
          --subsample_count;
          nalunitsum = 0;
        }
        else
          nalunitsum += nalsize + nal_length_size_;
      }
      if (packet_in != packet_in_e || subsample_count)
      {
        CLog::Log(LOGERROR, "NAL Unit definition incomplete (nls: %d) %d -> %u ", nal_length_size_, (int)(packet_in_e - packet_in), subsample_count);
        return AP4_ERROR_NOT_SUPPORTED;
      }
      data_out.SetDataSize(data_out.GetDataSize() + data_in.GetDataSize() + (4 - nal_length_size_) * nalunitcount);
    }else
      data_out.AppendData(data_in.GetData(), data_in.GetDataSize());
  }
  return AP4_SUCCESS;
}

extern "C" {

#ifdef _WIN32
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API
#endif

  class SSD_DECRYPTER MODULE_API *CreateDecryptorInstance(class SSD_HOST *h, uint32_t host_version)
  {
    if (host_version != SSD_HOST::version)
      return 0;
    return new WVDecrypter;
  };

  void MODULE_API DeleteDecryptorInstance(class SSD_DECRYPTER *d)
  {
    delete d;
  }
};
