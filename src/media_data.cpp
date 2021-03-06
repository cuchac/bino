/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
 * Joe <joe@wpj.cz>
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

#include <limits>
#include <cstring>
#include <cmath>

#include "media_data.h"

#include "str.h"
#include "msg.h"
#include "dbg.h"


video_frame::video_frame() :
    raw_width(-1),
    raw_height(-1),
    raw_aspect_ratio(0.0f),
    width(-1),
    height(-1),
    aspect_ratio(0.0f),
    layout(bgra32),
    color_space(srgb),
    value_range(u8_full),
    chroma_location(center),
    stereo_layout(mono),
    stereo_layout_swap(false),
    presentation_time(std::numeric_limits<int64_t>::min())
{
    for (int i = 0; i < 2; i++)
    {
        for (int p = 0; p < 3; p++)
        {
            data[i][p] = NULL;
            line_size[i][p] = 0;
        }
    }
}

void video_frame::set_view_dimensions()
{
    width = raw_width;
    height = raw_height;
    aspect_ratio = raw_aspect_ratio;
    if (stereo_layout == left_right)
    {
        width /= 2;
        aspect_ratio /= 2.0f;
    }
    else if (stereo_layout == left_right_half)
    {
        width /= 2;
    }
    else if (stereo_layout == top_bottom)
    {
        height /= 2;
        aspect_ratio *= 2.0f;
    }
    else if (stereo_layout == top_bottom_half)
    {
        height /= 2;
    }
    else if (stereo_layout == even_odd_rows)
    {
        height /= 2;
        //aspect_ratio *= 2.0f;
        // The only video files I know of which use row-alternating format (those from stereopia.com)
        // do not want this adjustment of aspect ratio.
    }
}

std::string video_frame::stereo_layout_to_string(stereo_layout_t stereo_layout, bool stereo_layout_swap)
{
    std::string s;
    switch (stereo_layout)
    {
    case mono:
        s = "mono";
        break;
    case separate:
        s = stereo_layout_swap ? "separate-right-left" : "separate-left-right";
        break;
    case top_bottom:
        s = stereo_layout_swap ? "bottom-top" : "top-bottom";
        break;
    case top_bottom_half:
        s = stereo_layout_swap ? "bottom-top-half" : "top-bottom-half";
        break;
    case left_right:
        s = stereo_layout_swap ? "right-left" : "left-right";
        break;
    case left_right_half:
        s = stereo_layout_swap ? "right-left-half" : "left-right-half";
        break;
    case even_odd_rows:
        s = stereo_layout_swap ? "odd-even-rows" : "even-odd-rows";
        break;
    }
    return s;
}

void video_frame::stereo_layout_from_string(const std::string &s, stereo_layout_t &stereo_layout, bool &stereo_layout_swap)
{
    if (s == "mono")
    {
        stereo_layout = mono;
        stereo_layout_swap = false;
    }
    else if (s == "separate-right-left")
    {
        stereo_layout = separate;
        stereo_layout_swap = true;
    }
    else if (s == "separate-left-right")
    {
        stereo_layout = separate;
        stereo_layout_swap = false;
    }
    else if (s == "bottom-top")
    {
        stereo_layout = top_bottom;
        stereo_layout_swap = true;
    }
    else if (s == "top-bottom")
    {
        stereo_layout = top_bottom;
        stereo_layout_swap = false;
    }
    else if (s == "bottom-top-half")
    {
        stereo_layout = top_bottom_half;
        stereo_layout_swap = true;
    }
    else if (s == "top-bottom-half")
    {
        stereo_layout = top_bottom_half;
        stereo_layout_swap = false;
    }
    else if (s == "right-left")
    {
        stereo_layout = left_right;
        stereo_layout_swap = true;
    }
    else if (s == "left-right")
    {
        stereo_layout = left_right;
        stereo_layout_swap = false;
    }
    else if (s == "right-left-half")
    {
        stereo_layout = left_right_half;
        stereo_layout_swap = true;
    }
    else if (s == "left-right-half")
    {
        stereo_layout = left_right_half;
        stereo_layout_swap = false;
    }
    else if (s == "odd-even-rows")
    {
        stereo_layout = even_odd_rows;
        stereo_layout_swap = true;
    }
    else if (s == "even-odd-rows")
    {
        stereo_layout = even_odd_rows;
        stereo_layout_swap = false;
    }
    else
    {
        // safe fallback
        stereo_layout = mono;
        stereo_layout_swap = false;
    }
}

std::string video_frame::format_name() const
{
    std::string name = str::asprintf("%dx%d-%.3g:1-", raw_width, raw_height, raw_aspect_ratio);
    switch (layout)
    {
    case bgra32:
        name += "bgra32";
        break;
    case yuv444p:
        name += "yuv444p";
        break;
    case yuv422p:
        name += "yuv422p";
        break;
    case yuv420p:
        name += "yuv420p";
        break;
    }
    switch (color_space)
    {
    case srgb:
        name += "-srgb";
        break;
    case yuv601:
        name += "-601";
        break;
    case yuv709:
        name += "-709";
        break;
    }
    if (layout != bgra32)
    {
        switch (value_range)
        {
        case u8_full:
            name += "-jpeg";
            break;
        case u8_mpeg:
            name += "-mpeg";
            break;
        }
    }
    if (layout == yuv422p || layout == yuv420p)
    {
        switch (chroma_location)
        {
        case center:
            name += "-c";
            break;
        case left:
            name += "-l";
            break;
        case topleft:
            name += "-tl";
            break;
        }
    }
    return name;
}

std::string video_frame::format_info() const
{
    return str::asprintf("%dx%d, %.3g:1", raw_width, raw_height, aspect_ratio);
}

static int next_multiple_of_4(int x)
{
    return (x / 4 + (x % 4 == 0 ? 0 : 1)) * 4;
}

void video_frame::copy_plane(int view, int plane, void *buf) const
{
    char *dst = reinterpret_cast<char *>(buf);
    const char *src = NULL;
    size_t src_offset = 0;
    size_t src_row_size = 0;
    size_t dst_row_width = 0;
    size_t dst_row_size = 0;
    size_t lines = 0;

    switch (layout)
    {
    case bgra32:
        dst_row_width = width * 4;
        dst_row_size = dst_row_width;
        lines = height;
        break;

    case yuv444p:
        dst_row_width = width;
        dst_row_size = next_multiple_of_4(dst_row_width);
        lines = height;
        break;

    case yuv422p:
        if (plane == 0)
        {
            dst_row_width = width;
            dst_row_size = next_multiple_of_4(dst_row_width);
            lines = height;
        }
        else
        {
            dst_row_width = width / 2;
            dst_row_size = next_multiple_of_4(dst_row_width);
            lines = height;
        }
        break;

    case yuv420p:
        if (plane == 0)
        {
            dst_row_width = width;
            dst_row_size = next_multiple_of_4(dst_row_width);
            lines = height;
        }
        else
        {
            dst_row_width = width / 2;
            dst_row_size = next_multiple_of_4(dst_row_width);
            lines = height / 2;
        }
        break;
    }

    if (stereo_layout_swap)
    {
        view = (view == 0 ? 1 : 0);
    }
    switch (stereo_layout)
    {
    case mono:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = line_size[0][plane];
        src_offset = 0;
        break;
    case separate:
        src = static_cast<const char *>(data[view][plane]);
        src_row_size = line_size[view][plane];
        src_offset = 0;
        break;
    case top_bottom:
    case top_bottom_half:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = line_size[0][plane];
        src_offset = view * lines * src_row_size;
        break;
    case left_right:
    case left_right_half:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = line_size[0][plane];
        src_offset = view * dst_row_width;
        break;
    case even_odd_rows:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = 2 * line_size[0][plane];
        src_offset = view * line_size[0][plane];
        break;
    }

    if (src_row_size == dst_row_size)
    {
        std::memcpy(dst, src + src_offset, lines * src_row_size);
    }
    else
    {
        size_t dst_offset = 0;
        for (size_t y = 0; y < lines; y++)
        {
            std::memcpy(dst + dst_offset, src + src_offset, dst_row_width);
            dst_offset += dst_row_size;
            src_offset += src_row_size;
        }
    }
}

audio_blob::audio_blob() :
    channels(-1),
    rate(-1),
    sample_format(u8),
    data(NULL),
    size(0),
    presentation_time(std::numeric_limits<int64_t>::min())
{
}

std::string audio_blob::format_info() const
{
    return str::asprintf("%d channels, %g kHz, %d bit", channels, rate / 1e3f, sample_bits());
}

std::string audio_blob::format_name() const
{
    const char *sample_format_name;
    switch (sample_format)
    {
    case u8:
        sample_format_name = "u8";
        break;
    case s16:
        sample_format_name = "s16";
        break;
    case f32:
        sample_format_name = "f32";
        break;
    case d64:
        sample_format_name = "d64";
        break;
    }
    return str::asprintf("%d-%d-%s", channels, rate, sample_format_name);
}

int audio_blob::sample_bits() const
{
    int bits = 0;
    switch (sample_format)
    {
    case u8:
        bits = 8;
        break;
    case s16:
        bits = 16;
        break;
    case f32:
        bits = 32;
        break;
    case d64:
        bits = 64;
        break;
    }
    return bits;
}

subtitle_box::subtitle_box() :
    format(text),
    language(),
    str(),
    presentation_start_time(std::numeric_limits<int64_t>::min()),
    presentation_stop_time(std::numeric_limits<int64_t>::min())
{
}

std::string subtitle_box::format_info() const
{
    std::string s;
    if (language.empty())
    {
        s += "Unknown ";
    }
    else
    {
        s += language + " ";
    }
    switch (format)
    {
    case text:
        s += "(text format)";
    }
    return s;
}

std::string subtitle_box::format_name() const
{
    std::string s;
    if (language.empty())
    {
        s += "unknown-";
    }
    else
    {
        s += language + "-";
    }
    switch (format)
    {
    case text:
        s += "text";
    }
    return s;
}

parameters::parameters() :
    stereo_mode(stereo),
    stereo_mode_swap(false),
    parallax(std::numeric_limits<float>::quiet_NaN()),
    crosstalk_r(std::numeric_limits<float>::quiet_NaN()),
    crosstalk_g(std::numeric_limits<float>::quiet_NaN()),
    crosstalk_b(std::numeric_limits<float>::quiet_NaN()),
    ghostbust(std::numeric_limits<float>::quiet_NaN()),
    contrast(std::numeric_limits<float>::quiet_NaN()),
    brightness(std::numeric_limits<float>::quiet_NaN()),
    hue(std::numeric_limits<float>::quiet_NaN()),
    saturation(std::numeric_limits<float>::quiet_NaN()),
    subtitles_color(-1),
    subtitles_encoding(""),
    subtitles_font("")
{
}

void parameters::set_defaults()
{
    if (!std::isfinite(parallax) || parallax < -1.0f || parallax > +1.0f)
    {
        parallax = 0.0f;
    }
    if (!std::isfinite(crosstalk_r) || crosstalk_r < 0.0f || crosstalk_r > +1.0f)
    {
        crosstalk_r = 0.0f;
    }
    if (!std::isfinite(crosstalk_g) || crosstalk_g < 0.0f || crosstalk_g > +1.0f)
    {
        crosstalk_g = 0.0f;
    }
    if (!std::isfinite(crosstalk_b) || crosstalk_b < 0.0f || crosstalk_b > +1.0f)
    {
        crosstalk_b = 0.0f;
    }
    if (!std::isfinite(ghostbust) || ghostbust < 0.0f || ghostbust > +1.0f)
    {
        ghostbust = 0.0f;
    }
    if (!std::isfinite(contrast) || contrast < -1.0f || contrast > +1.0f)
    {
        contrast = 0.0f;
    }
    if (!std::isfinite(brightness) || brightness < -1.0f || brightness > +1.0f)
    {
        brightness = 0.0f;
    }
    if (!std::isfinite(hue) || hue < -1.0f || hue > +1.0f)
    {
        hue = 0.0f;
    }
    if (!std::isfinite(saturation) || saturation < -1.0f || saturation > +1.0f)
    {
        saturation = 0.0f;
    }
    if (subtitles_color < 0 || subtitles_color > 0x00FFFFFF)
    {
       subtitles_color = 0x00FFFFFF;
    }
    if (subtitles_encoding.empty())
    {
        subtitles_encoding = "UTF-8";
    }
    if (subtitles_font.empty())
    {
       subtitles_font = "";
    }
}

std::string parameters::stereo_mode_to_string(stereo_mode_t stereo_mode, bool stereo_mode_swap)
{
    std::string s;
    switch (stereo_mode)
    {
    case stereo:
        s = "stereo";
        break;
    case mono_left:
        s = "mono-left";
        break;
    case mono_right:
        s = "mono-right";
        break;
    case top_bottom:
        s = "top-bottom";
        break;
    case top_bottom_half:
        s = "top-bottom-half";
        break;
    case left_right:
        s = "left-right";
        break;
    case left_right_half:
        s = "left-right-half";
        break;
    case even_odd_rows:
        s = "even-odd-rows";
        break;
    case even_odd_columns:
        s = "even-odd-columns";
        break;
    case checkerboard:
        s = "checkerboard";
        break;
    case red_cyan_monochrome:
        s = "red-cyan-monochrome";
        break;
    case red_cyan_half_color:
        s = "red-cyan-half-color";
        break;
    case red_cyan_full_color:
        s = "red-cyan-full-color";
        break;
    case red_cyan_dubois:
        s = "red-cyan-dubois";
        break;
    case green_magenta_monochrome:
        s = "green-magenta-monochrome";
        break;
    case green_magenta_half_color:
        s = "green-magenta-half-color";
        break;
    case green_magenta_full_color:
        s = "green-magenta-full-color";
        break;
    case green_magenta_dubois:
        s = "green-magenta-dubois";
        break;
    case amber_blue_monochrome:
        s = "amber-blue-monochrome";
        break;
    case amber_blue_half_color:
        s = "amber-blue-half-color";
        break;
    case amber_blue_dubois:
        s = "amber-blue-dubois";
        break;
    case amber_blue_full_color:
        s = "amber-blue-full-color";
        break;
    case red_green_monochrome:
        s = "red-green-monochrome";
        break;
    case red_blue_monochrome:
        s = "red-blue-monochrome";
        break;
    }
    if (stereo_mode_swap)
    {
        s += "-swap";
    }
    return s;
}

void parameters::stereo_mode_from_string(const std::string &s, stereo_mode_t &stereo_mode, bool &stereo_mode_swap)
{
    size_t x = s.find_last_of("-");
    std::string t;
    if (x != std::string::npos && s.substr(x) == "-swap")
    {
        t = s.substr(0, x);
        stereo_mode_swap = true;
    }
    else
    {
        t = s;
        stereo_mode_swap = false;
    }
    if (t == "stereo")
    {
        stereo_mode = stereo;
    }
    else if (t == "mono-left")
    {
        stereo_mode = mono_left;
    }
    else if (t == "mono-right")
    {
        stereo_mode = mono_right;
    }
    else if (t == "top-bottom")
    {
        stereo_mode = top_bottom;
    }
    else if (t == "top-bottom-half")
    {
        stereo_mode = top_bottom_half;
    }
    else if (t == "left-right")
    {
        stereo_mode = left_right;
    }
    else if (t == "left-right-half")
    {
        stereo_mode = left_right_half;
    }
    else if (t == "even-odd-rows")
    {
        stereo_mode = even_odd_rows;
    }
    else if (t == "even-odd-columns")
    {
        stereo_mode = even_odd_columns;
    }
    else if (t == "checkerboard")
    {
        stereo_mode = checkerboard;
    }
    else if (t == "red-cyan-monochrome")
    {
        stereo_mode = red_cyan_monochrome;
    }
    else if (t == "red-cyan-half-color")
    {
        stereo_mode = red_cyan_half_color;
    }
    else if (t == "red-cyan-full-color")
    {
        stereo_mode = red_cyan_full_color;
    }
    else if (t == "red-cyan-dubois")
    {
        stereo_mode = red_cyan_dubois;
    }
    else if (t == "green-magenta-monochrome")
    {
        stereo_mode = green_magenta_monochrome;
    }
    else if (t == "green-magenta-half-color")
    {
        stereo_mode = green_magenta_half_color;
    }
    else if (t == "green-magenta-full-color")
    {
        stereo_mode = green_magenta_full_color;
    }
    else if (t == "green-magenta-dubois")
    {
        stereo_mode = green_magenta_dubois;
    }
    else if (t == "amber-blue-monochrome")
    {
        stereo_mode = amber_blue_monochrome;
    }
    else if (t == "amber-blue-half-color")
    {
        stereo_mode = amber_blue_half_color;
    }
    else if (t == "amber-blue-full-color")
    {
        stereo_mode = amber_blue_full_color;
    }
    else if (t == "amber-blue-dubois")
    {
        stereo_mode = amber_blue_dubois;
    }
    else if (t == "red-green-monochrome")
    {
        stereo_mode = red_green_monochrome;
    }
    else if (t == "red-blue-monochrome")
    {
        stereo_mode = red_blue_monochrome;
    }
    else
    {
        // safe fallback
        stereo_mode = mono_left;
    }
}

void parameters::save(std::ostream &os) const
{
    s11n::save(os, static_cast<int>(stereo_mode));
    s11n::save(os, stereo_mode_swap);
    s11n::save(os, parallax);
    s11n::save(os, crosstalk_r);
    s11n::save(os, crosstalk_g);
    s11n::save(os, crosstalk_b);
    s11n::save(os, ghostbust);
    s11n::save(os, contrast);
    s11n::save(os, brightness);
    s11n::save(os, hue);
    s11n::save(os, saturation);
    s11n::save(os, subtitles_color);
    s11n::save(os, subtitles_font);
    s11n::save(os, subtitles_encoding);
}

void parameters::load(std::istream &is)
{
    int x;
    s11n::load(is, x);
    stereo_mode = static_cast<stereo_mode_t>(x);
    s11n::load(is, stereo_mode_swap);
    s11n::load(is, parallax);
    s11n::load(is, crosstalk_r);
    s11n::load(is, crosstalk_g);
    s11n::load(is, crosstalk_b);
    s11n::load(is, ghostbust);
    s11n::load(is, contrast);
    s11n::load(is, brightness);
    s11n::load(is, hue);
    s11n::load(is, saturation);
    s11n::load(is, subtitles_color);
    s11n::load(is, subtitles_font);
    s11n::load(is, subtitles_encoding);
}
