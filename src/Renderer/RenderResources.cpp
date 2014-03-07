/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#include "RenderResources.h"

namespace TrenchBroom {
    namespace Renderer {
        RenderResources::RenderResources(const GLAttribs& glAttribs, const wxGLContext* sharedContext) :
        m_glAttribs(glAttribs),
        m_sharedContext(sharedContext) {}
        
        const RenderResources::GLAttribs& RenderResources::glAttribs() const {
            return m_glAttribs;
        }
        
        const wxGLContext* RenderResources::sharedContext() const {
            return m_sharedContext;
        }
        
        FontManager& RenderResources::fontManager() {
            return m_fontManager;
        }
        
        ShaderManager& RenderResources::shaderManager() {
            return m_shaderManager;
        }
    }
}
