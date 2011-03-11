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

#ifndef MEDIA_OBJECT_H
#define MEDIA_OBJECT_H

#include <string>
#include <vector>

#include "media_data.h"


class media_object
{
private:
    std::string _url;                           // The URL of the media object (may be a file)
    struct ffmpeg_stuff *_ffmpeg;               // FFmpeg related data
    std::vector<std::string> _tag_names;        // Meta data: tag names
    std::vector<std::string> _tag_values;       // Meta data: tag values

    // Set video frame and audio blob templates by extracting the information
    // from the given streams
    void set_video_frame_template(int video_stream);
    void set_audio_blob_template(int audio_stream);
	 void set_subtitles_list_template(int subtitles_stream);

    // The threaded implementation can access private members
    friend class read_thread;
    friend class video_decode_thread;
    friend class audio_decode_thread;

public:

    /* Constructor, Destructor */
    media_object();
    ~media_object();

    /*
     * Initialization
     */

    /* Open a media object. The URL may simply be a file name. */
    void open(const std::string &url);

    /* Get metadata */
    const std::string &url() const;
    size_t tags() const;
    const std::string &tag_name(size_t i) const;
    const std::string &tag_value(size_t i) const;
    const std::string &tag_value(const std::string &tag_name) const;

    /* Get the number of video and audio streams in the file. */
    int video_streams() const;
    int audio_streams() const;
	 int subtitles_streams() const;

    /* Activate a video or audio stream for usage. Inactive streams will not be accessible. */
    void video_stream_set_active(int video_stream, bool active);
    void audio_stream_set_active(int audio_stream, bool active);
	 void subtitles_stream_set_active(int subtitles_stream, bool active);

    /* Get information about video streams. */
    // Return a video frame with all properties filled in (but without any data).
    // Note that this is only a hint; the properties of actual video frames may differ!
    const video_frame &video_frame_template(int video_stream) const;
    // Return frame rate. This is just informal; usually, the presentation time of each frame
    // should be used (videos do not need to have constant frame rate).
    int video_frame_rate_numerator(int video_stream) const;
    int video_frame_rate_denominator(int video_stream) const;
    // Video stream duration in microseconds.
    int64_t video_duration(int video_stream) const;

    /* Get information about audio streams. */
    // Return an audio blob with all properties filled in (but without any data).
    // Note that this is only a hint; the properties of actual audio blobs may differ!
    const audio_blob &audio_blob_template(int audio_stream) const;
    // Audio stream duration in microseconds.
    int64_t audio_duration(int video_stream) const;
    
    /* Get information about subtitles streams. */
    // Return an subtitles blob with all properties filled in (but without any data).
    // Note that this is only a hint; the properties of actual subtitles blobs may differ!
    subtitles_list &subtitles_list_template(int subtitles_stream) const;

    /*
     * Access video and audio data
     */

    /* Start to read a video frame asynchronously (in a separate thread). */
    void start_video_frame_read(int video_stream);
    /* Wait for the video frame reading to finish, and return the frame.
     * An invalid frame means that EOF was reached. */
    video_frame finish_video_frame_read(int video_stream);

    /* Start to read the given amount of audio data asynchronously (in a separate thread). */
    void start_audio_blob_read(int audio_stream, size_t size);
    /* Wait for the audio data reading to finish, and return the blob.
     * An invalid blob means that EOF was reached. */
    audio_blob finish_audio_blob_read(int audio_stream);
    
    /* Start to read the given amount of subtitles data asynchronously (in a separate thread). */
    void start_subtitles_list_read(int subtitles_stream, size_t size);
    /* Wait for the subtitles data reading to finish, and return the subtitles_list.
     * An invalid subtitles_list means that EOF was reached. */
    subtitles_list* finish_subtitles_list_read(int subtitles_stream);

    /* Return the last position in microseconds, of the last packet that was read in any
     * stream. If the position is unkown, the minimum possible value is returned. */
    int64_t tell();

    /* Seek to the given position in microseconds. This affects all streams.
     * Make sure that the position is not out of range!
     * The real position after seeking is only revealed after reading the next video frame
     * or audio blob. This position may differ from the requested position for various
     * reasons (seeking is only possible to keyframes, seeking is not supported by the
     * stream, ...) */
    void seek(int64_t pos);

    /*
     * Cleanup
     */

    /* When done, close the object and clean up. */
    void close();
};

#endif
