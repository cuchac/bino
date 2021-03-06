/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#ifdef __APPLE__
#  include <OpenAL/al.h>
#  include <OpenAL/alc.h>
#else
#  include <AL/al.h>
#  include <AL/alc.h>
#endif

#include <GL/glew.h>

#include <QGLWidget>

#if HAVE_LIBEQUALIZER
#include <eq/eq.h>
#endif

#include "qt_app.h"

#include "str.h"


static std::vector<std::string> ffmpeg_v;
static std::vector<std::string> openal_v;
static std::vector<std::string> opengl_v;
static std::vector<std::string> glew_v;
static std::vector<std::string> equalizer_v;
static std::vector<std::string> qt_v;

static void ffmpeg_versions()
{
    if (ffmpeg_v.size() == 0)
    {
        ffmpeg_v.push_back(str::asprintf("libavformat %d.%d.%d / %d.%d.%d",
                    LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
                    avformat_version() >> 16, avformat_version() >> 8 & 0xff, avformat_version() & 0xff));
        ffmpeg_v.push_back(str::asprintf("libavcodec %d.%d.%d / %d.%d.%d",
                    LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO,
                    avcodec_version() >> 16, avcodec_version() >> 8 & 0xff, avcodec_version() & 0xff));
        ffmpeg_v.push_back(str::asprintf("libswscale %d.%d.%d / %d.%d.%d",
                    LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO,
                    swscale_version() >> 16, swscale_version() >> 8 & 0xff, swscale_version() & 0xff));
    }
}

void set_openal_versions()
{
    if (openal_v.size() == 0)
    {
        openal_v.push_back(std::string("Version ") + static_cast<const char *>(alGetString(AL_VERSION)));
        openal_v.push_back(std::string("Renderer ") + static_cast<const char *>(alGetString(AL_RENDERER)));
        openal_v.push_back(std::string("Vendor ") + static_cast<const char *>(alGetString(AL_VENDOR)));
    }
}

static void openal_versions()
{
    if (openal_v.size() == 0)
    {
        ALCdevice *device = alcOpenDevice(NULL);
        if (device)
        {
            ALCcontext *context = alcCreateContext(device, NULL);
            if (context)
            {
                alcMakeContextCurrent(context);
                set_openal_versions();
                alcMakeContextCurrent(NULL);
                alcDestroyContext(context);
            }
            alcCloseDevice(device);
        }
        if (openal_v.size() == 0)
        {
            openal_v.push_back("unknown");
        }
    }
}

void set_opengl_versions()
{
    if (opengl_v.size() == 0)
    {
        opengl_v.push_back(std::string("Version ") + reinterpret_cast<const char *>(glGetString(GL_VERSION)));
        opengl_v.push_back(std::string("Renderer ") + reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
        opengl_v.push_back(std::string("Vendor ") + reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
    }
}

static void opengl_versions()
{
    if (opengl_v.size() == 0)
    {
#ifdef Q_WS_X11
        const char *display = getenv("DISPLAY");
        bool have_display = (display && display[0] != '\0');
#else
        bool have_display = true;
#endif
        if (have_display)
        {
            bool qt_app_owner = init_qt();
            QGLWidget *tmpwidget = new QGLWidget();
            tmpwidget->makeCurrent();
            set_opengl_versions();
            delete tmpwidget;
            if (qt_app_owner)
            {
                exit_qt();
            }
        }
        if (opengl_v.size() == 0)
        {
            opengl_v.push_back("unknown");
        }
    }
}

static void glew_versions()
{
    if (glew_v.size() == 0)
    {
        glew_v.push_back(reinterpret_cast<const char *>(glewGetString(GLEW_VERSION)));
    }
}

static void equalizer_versions()
{
    if (equalizer_v.size() == 0)
    {
#if HAVE_LIBEQUALIZER
        equalizer_v.push_back(str::asprintf("%d.%d.%d / %d.%d.%d",
                    EQ_VERSION_MAJOR, EQ_VERSION_MINOR, EQ_VERSION_PATCH,
                    static_cast<int>(eq::Version::getMajor()),
                    static_cast<int>(eq::Version::getMinor()),
                    static_cast<int>(eq::Version::getPatch())));
#else
        equalizer_v.push_back("not used");
#endif
    }
}

static void qt_versions()
{
    if (qt_v.size() == 0)
    {
        qt_v.push_back(std::string(QT_VERSION_STR) + " / " + qVersion());
    }
}

std::vector<std::string> lib_versions(bool html)
{
    ffmpeg_versions();
    openal_versions();
    opengl_versions();
    glew_versions();
    equalizer_versions();
    qt_versions();

    std::vector<std::string> v;
    if (html)
    {
        v.push_back("<ul>");
        v.push_back("<li><a href=\"http://ffmpeg.org/\">FFmpeg</a>");
        for (size_t i = 0; i < ffmpeg_v.size(); i++)
        {
            v.push_back(std::string("<br>") + ffmpeg_v[i]);
        }
        v.push_back("</li>");
        v.push_back("<li><a href=\"http://kcat.strangesoft.net/openal.html\">OpenAL</a>");
        for (size_t i = 0; i < openal_v.size(); i++)
        {
            v.push_back(std::string("<br>") + openal_v[i]);
        }
        v.push_back("</li>");
        v.push_back("<li><a href=\"http://www.opengl.org/\">OpenGL</a>");
        for (size_t i = 0; i < opengl_v.size(); i++)
        {
            v.push_back(std::string("<br>") + opengl_v[i]);
        }
        v.push_back("</li>");
        v.push_back("<li><a href=\"http://glew.sourceforge.net/\">GLEW</a>");
        for (size_t i = 0; i < glew_v.size(); i++)
        {
            v.push_back(std::string("<br>") + glew_v[i]);
        }
        v.push_back("</li>");
        v.push_back("<li><a href=\"http://www.equalizergraphics.com/\">Equalizer</a>");
        for (size_t i = 0; i < equalizer_v.size(); i++)
        {
            v.push_back(std::string("<br>") + equalizer_v[i]);
        }
        v.push_back("</li>");
        v.push_back("<li><a href=\"http://qt.nokia.com/\">Qt</a>");
        for (size_t i = 0; i < qt_v.size(); i++)
        {
            v.push_back(std::string("<br>") + qt_v[i]);
        }
        v.push_back("</li>");
        v.push_back("</ul>");
    }
    else
    {
        v.push_back("FFmpeg:");
        for (size_t i = 0; i < ffmpeg_v.size(); i++)
        {
            v.push_back(std::string("    ") + ffmpeg_v[i]);
        }
        v.push_back("OpenAL:");
        for (size_t i = 0; i < openal_v.size(); i++)
        {
            v.push_back(std::string("    ") + openal_v[i]);
        }
        v.push_back("OpenGL:");
        for (size_t i = 0; i < opengl_v.size(); i++)
        {
            v.push_back(std::string("    ") + opengl_v[i]);
        }
        v.push_back("GLEW:");
        for (size_t i = 0; i < glew_v.size(); i++)
        {
            v.push_back(std::string("    ") + glew_v[i]);
        }
        v.push_back("Equalizer:");
        for (size_t i = 0; i < equalizer_v.size(); i++)
        {
            v.push_back(std::string("    ") + equalizer_v[i]);
        }
        v.push_back("Qt:");
        for (size_t i = 0; i < qt_v.size(); i++)
        {
            v.push_back(std::string("    ") + qt_v[i]);
        }
    }

    return v;
}
