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

#include "VideoPlayerMessenger.h"

#include <memory>
#include <utility>

#include "VideoPlayer.h"
#include "threads/SingleLock.h"

namespace KODI
{
namespace VIDEOPLAYER
{

CDelayedMessage::CDelayedMessage(KODI::MESSAGING::ThreadMessage& msg, unsigned int delay) : CThread("DelayedMessage")
{
  m_msg = msg;

  m_delay = delay;
}

void CDelayedMessage::Process()
{
  Sleep(m_delay);

  if (!m_bStop)
    CVideoPlayerMessenger::GetInstance().PostMsg(m_msg.dwMessage, m_msg.param1, m_msg.param1, m_msg.lpVoid, m_msg.strParam, m_msg.params);
}


CVideoPlayerMessenger& CVideoPlayerMessenger::GetInstance()
{
  static CVideoPlayerMessenger appMessenger;
  return appMessenger;
}

void CVideoPlayerMessenger::setMainThreadId(pthread_t tid)
{
  m_mainThreadId = tid;
}

bool CVideoPlayerMessenger::IsCurrentThread() const
{
  return pthread_equal(pthread_self(), m_mainThreadId);
}

CVideoPlayerMessenger::CVideoPlayerMessenger()
{
}

CVideoPlayerMessenger::~CVideoPlayerMessenger()
{
  Cleanup();
}

void CVideoPlayerMessenger::Cleanup()
{
  CSingleLock lock (m_critSection);

  while (!m_vecMessages.empty())
  {
    KODI::MESSAGING::ThreadMessage* pMsg = m_vecMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecMessages.pop();
  }
}

int CVideoPlayerMessenger::SendMsg(KODI::MESSAGING::ThreadMessage&& message, bool wait)
{
  std::shared_ptr<CEvent> waitEvent;
  std::shared_ptr<int> result;

  if (wait)
  { 
    //Initialize result here as it's not needed for posted messages
    message.result = std::make_shared<int>(-1);
    // check that we're not being called from our application thread, else we'll be waiting
    // forever!
    if (!IsCurrentThread())
    {
      message.waitEvent.reset(new CEvent(true));
      waitEvent = message.waitEvent;
      result = message.result;
    }
    else
    {
      //OutputDebugString("Attempting to wait on a SendMessage() from our application thread will cause lockup!\n");
      //OutputDebugString("Sending immediately\n");
      ProcessMessage(&message);
      return *message.result;
    }
  }

  KODI::MESSAGING::ThreadMessage* msg = new KODI::MESSAGING::ThreadMessage(std::move(message));
  
  CSingleLock lock (m_critSection);

  m_vecMessages.push(msg);
  lock.Leave();  // this releases the lock on the vec of messages and
                 //   allows the ProcessMessage to execute and therefore
                 //   delete the message itself. Therefore any accesss
                 //   of the message itself after this point consittutes
                 //   a race condition (yarc - "yet another race condition")
                 //
  if (waitEvent) // ... it just so happens we have a spare reference to the
                 //  waitEvent ... just for such contingencies :)
  { 
    waitEvent->Wait();
    return *result;
  }

  return -1;
}

int CVideoPlayerMessenger::SendMsg(uint32_t messageId)
{
   return SendMsg(KODI::MESSAGING::ThreadMessage{ messageId }, true);
}

int CVideoPlayerMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload)
{
  return SendMsg(KODI::MESSAGING::ThreadMessage{ messageId, param1, param2, payload }, true);
}

int CVideoPlayerMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam)
{
  return SendMsg(KODI::MESSAGING::ThreadMessage{ messageId, param1, param2, payload, strParam, std::vector<std::string>{} }, true);
}

int CVideoPlayerMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params)
{
  return SendMsg(KODI::MESSAGING::ThreadMessage{ messageId, param1, param2, payload, strParam, params }, true);
}

void CVideoPlayerMessenger::PostMsg(uint32_t messageId)
{
  SendMsg(KODI::MESSAGING::ThreadMessage{ messageId }, false);
}

void CVideoPlayerMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload)
{
  SendMsg(KODI::MESSAGING::ThreadMessage{ messageId, param1, param2, payload }, false);
}

void CVideoPlayerMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam)
{
  SendMsg(KODI::MESSAGING::ThreadMessage{ messageId, param1, param2, payload, strParam, std::vector<std::string>{} }, false);
}

void CVideoPlayerMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params)
{
  SendMsg(KODI::MESSAGING::ThreadMessage{ messageId, param1, param2, payload, strParam, params }, false);
}

void CVideoPlayerMessenger::ProcessMessages()
{
  // process threadmessages
  CSingleLock lock (m_critSection);
  while (!m_vecMessages.empty())
  {
    KODI::MESSAGING::ThreadMessage* pMsg = m_vecMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecMessages.pop();

    //Leave here as the message might make another
    //thread call processmessages or sendmessage

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    
    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    lock.Enter();
  }
}

void CVideoPlayerMessenger::ProcessMessage(KODI::MESSAGING::ThreadMessage *pMsg)
{
  //special case for this that we handle ourselves
  if (pMsg->dwMessage == TMSG_CALLBACK)
  {
    ThreadMessageCallback *callback = static_cast<ThreadMessageCallback*>(pMsg->lpVoid);
    callback->callback(callback->userptr);
    return;
  }

  CSingleLock lock(m_critSection);
  int mask = pMsg->dwMessage & TMSG_MASK_MESSAGE;

  auto target = m_mapTargets.at(mask);
  if (target != nullptr)
  {
    CSingleExit exit(m_critSection);
    target->OnApplicationMessage(pMsg);
  }
}

void CVideoPlayerMessenger::RegisterReceiver(CVideoPlayer* target)
{
  CSingleLock lock(m_critSection);
  m_mapTargets.insert(std::make_pair(target->GetMessageMask(), target));
}

}
}
