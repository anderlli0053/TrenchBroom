/*
 Copyright (C) 2010-2016 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TextCtrlOutputAdapter.h"

#include <wx/textctrl.h>
#include <wx/wupdlock.h>

#include <cassert>

namespace TrenchBroom {
    namespace View {
        TextCtrlOutputAdapter::TextCtrlOutputAdapter(wxTextCtrl* textCtrl) :
        m_textCtrl(textCtrl),
        m_lastNewLine(0) {
            assert(m_textCtrl != NULL);
            bindEvents();
        }

        TextCtrlOutputAdapter::TextCtrlOutputAdapter(const TextCtrlOutputAdapter& other) :
        m_textCtrl(other.m_textCtrl),
        m_lastNewLine(other.m_lastNewLine) {
            assert(m_textCtrl != NULL);
            bindEvents();
        }
        
        TextCtrlOutputAdapter::~TextCtrlOutputAdapter() {
            unbindEvents();
        }

        TextCtrlOutputAdapter& TextCtrlOutputAdapter::operator=(const TextCtrlOutputAdapter& other) {
            m_textCtrl = other.m_textCtrl;
            m_lastNewLine = other.m_lastNewLine;
            assert(m_textCtrl != NULL);
            bindEvents();
            return *this;
        }

        void TextCtrlOutputAdapter::sendAppendEvent(const wxString& str) {
            wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD, m_textCtrl->GetId());
            event->SetString(str);
            m_textCtrl->GetEventHandler()->QueueEvent(event);
        }

        void TextCtrlOutputAdapter::OnAsyncAppend(wxThreadEvent& event) {
            const wxString str = compressString(event.GetString());
            if (!str.IsEmpty()) {
                wxWindowUpdateLocker lock(m_textCtrl);
                
                size_t l = 0;
                for (size_t i = 0; i < str.Len(); ++i) {
                    const wxUniChar c = str[i];
                    if (c == '\r') {
                        const long from = static_cast<long>(m_lastNewLine);
                        const long to   = m_textCtrl->GetLastPosition();
                        m_textCtrl->Remove(from, to);
                        l = i;
                    } else if (c == '\n') {
                        m_textCtrl->AppendText(str.Mid(l, i-l));
                        m_lastNewLine = static_cast<size_t>(m_textCtrl->GetLastPosition());
                        l = i;
                    }
                }
                m_textCtrl->AppendText(str.Mid(l));
            }
        }
        
        wxString TextCtrlOutputAdapter::compressString(const wxString& str) {
            wxString fullStr = m_remainder + str;
            wxString result;
            size_t chunkStart = 0;
            size_t previousChunkStart = 0;
            for (size_t i = 0; i < fullStr.Len(); ++i) {
                const wxUniChar c = fullStr[i];
                if (c == '\r') {
                    previousChunkStart = chunkStart;
                    chunkStart = i;
                } else if (c == '\n') {
                    result << fullStr.Mid(chunkStart, i - chunkStart + 1);
                    chunkStart = previousChunkStart = i+1;
                }
            }
            if (previousChunkStart < chunkStart) {
                const wxString chunk = fullStr.Mid(previousChunkStart, chunkStart - previousChunkStart);
                result << chunk;
            }
            m_remainder = fullStr.Mid(chunkStart);
            return result;
        }

        void TextCtrlOutputAdapter::bindEvents() {
            m_textCtrl->Bind(wxEVT_THREAD, &TextCtrlOutputAdapter::OnAsyncAppend, this, m_textCtrl->GetId());
        }
        
        void TextCtrlOutputAdapter::unbindEvents() {
            m_textCtrl->Unbind(wxEVT_THREAD, &TextCtrlOutputAdapter::OnAsyncAppend, this, m_textCtrl->GetId());
        }
    }
}
