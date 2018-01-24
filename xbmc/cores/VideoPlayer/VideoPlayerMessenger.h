#pragma once

/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://kodi.tv
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
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/WindowIDs.h"
#include "threads/Thread.h"
#include "messaging/ThreadMessage.h"

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#define TMSG_MASK_MESSAGE                 0xFFFF0000 // only keep the high bits to route messages
#define TMSG_MASK_VIDEOPLAYER             (1<<25)


#define TMSG_VP_RENDERER_FLUSH            TMSG_MASK_VIDEOPLAYER + 0
#define TMSG_VP_RENDERER_PREINIT          TMSG_MASK_VIDEOPLAYER + 1
#define TMSG_VP_RENDERER_UNINIT           TMSG_MASK_VIDEOPLAYER + 2

#define TMSG_CALLBACK                     800



class CGUIMessage;
class CVideoPlayer;

namespace KODI
{
namespace VIDEOPLAYER
{

class CDelayedMessage : public CThread
{
  public:
    CDelayedMessage(KODI::MESSAGING::ThreadMessage& msg, unsigned int delay);
    virtual void Process() override;

  private:
    unsigned int   m_delay;
    KODI::MESSAGING::ThreadMessage  m_msg;
};

struct ThreadMessageCallback
{
  void (*callback)(void *userptr);
  void *userptr;
};

class CVideoPlayerMessenger
{
public:
  /*!
   \brief The only way through which the global instance of the CVideoPlayerMessenger should be accessed.
   \return the global instance.
   */
  static CVideoPlayerMessenger& GetInstance();
  void setMainThreadId(pthread_t tid);
  bool IsCurrentThread() const;

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  int SendMsg(uint32_t messageId);
  int SendMsg(uint32_t messageId, int param1, int param2 = -1, void* payload = nullptr);
  int SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam);
  int SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params);

  void PostMsg(uint32_t messageId);
  void PostMsg(uint32_t messageId, int param1, int param2 = -1, void* payload = nullptr);
  void PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam);
  void PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params);

  void ProcessMessages(); // only call from main thread.

  void RegisterReceiver(CVideoPlayer* target);

private:
  // private construction, and no assignments; use the provided singleton methods
  CVideoPlayerMessenger();
  CVideoPlayerMessenger(const CVideoPlayerMessenger&) = delete;
  CVideoPlayerMessenger const& operator=(CVideoPlayerMessenger const&) = delete;
  ~CVideoPlayerMessenger();

  int SendMsg(KODI::MESSAGING::ThreadMessage&& msg, bool wait);
  void ProcessMessage(KODI::MESSAGING::ThreadMessage *pMsg);

  pthread_t m_mainThreadId;
  std::queue<KODI::MESSAGING::ThreadMessage*> m_vecMessages;
  std::map<int, CVideoPlayer*> m_mapTargets;
  CCriticalSection m_critSection;
};
}
}


