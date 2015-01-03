/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#extension GL_OES_EGL_image_external : require

precision mediump float;
uniform samplerExternalOES m_samp0;
varying vec4      m_cord0;
uniform float     m_blendoffset;

// SM_TEXTURE_RGBA_OES_BLEND
void main ()
{
    vec2 above, below;

    above.x = m_cord0.x;
    above.y = m_cord0.y - m_blendoffset;
    below.x = m_cord0.x;
    below.y = m_cord0.y + m_blendoffset;

    gl_FragColor.rgba = texture2D(m_samp0, above).rgba * 0.3 + texture2D(m_samp0, m_cord0.xy).rgba * 0.4 + texture2D(m_samp0, below).rgba * 0.3;
}
