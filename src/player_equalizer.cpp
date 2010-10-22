#include "config.h"

#include <sstream>

#include <eq/eq.h>

#include "debug.h"
#include "msg.h"
#include "s11n.h"

#include "video_output_opengl.h"
#include "player_equalizer.h"


/*
 * player_eq_node
 *
 * Implementation of player for eq_node.
 * The player_eq_node instance of the application node lives in eq_config instead of eq_node.
 */

class player_eq_node : public player
{
private:
    bool _is_master;
    video_frame_format _fmt;

public:
    player_eq_node() : player(player::slave), _is_master(false)
    {
    }

    void eq_make_master()
    {
        make_master();
        _is_master = true;
    }

    bool eq_init(const player_init_data &init_data,
            int *src_width, int *src_height, float *src_aspect_ratio, video_frame_format *src_preferred_frame_format)
    {
        try
        {
            create_decoders(init_data.filenames);
            create_input(init_data.input_mode);
            get_input_info(src_width, src_height, src_aspect_ratio, src_preferred_frame_format);
            _fmt = *src_preferred_frame_format;
            if (_is_master)
            {
                create_audio_output();
            }
        }
        catch (std::exception &e)
        {
            msg::err("%s", e.what());
            return false;
        }
        return true;
    }

    void eq_prep_frame(video_output *vo)
    {
        prepare_video_frame(vo);
    }

    void eq_run_step(bool *more_steps, bool *prep_frame, bool *drop_frame, bool *display_frame)
    {
        run_step(more_steps, prep_frame, drop_frame, display_frame);
    }

    void eq_get_frame()
    {
        get_video_frame(_fmt);
    }

    void eq_release_frame()
    {
        release_video_frame();
    }
};

/*
 * video_output_opengl_eq_window
 *
 * Implementation of video_output_opengl for eq_window
 */

class video_output_opengl_eq_window : public video_output_opengl
{
public:
    video_output_opengl_eq_window() : video_output_opengl()
    {
    }

    void eq_display(video_output::mode mode, float x, float y, float w, float h)
    {
        // TODO: x, y, w, h describe the quad, set by the channel
        video_output_opengl::display(mode);
    }

    void eq_initialize(int src_width, int src_height, float src_aspect_ratio, video_frame_format src_preferred_frame_format,
            bool have_pixel_buffer_object, bool have_texture_non_power_of_two, bool have_fragment_shader)
    {
        set_mode(stereo);       // just to ensure that prepare() does the right thing
        set_source_info(src_width, src_height, src_aspect_ratio, src_preferred_frame_format);
        initialize(have_pixel_buffer_object, have_texture_non_power_of_two, have_fragment_shader);
    }

    void eq_deinitialize()
    {
        deinitialize();
    }

    void eq_swap_tex_set()
    {
        swap_tex_set();
    }

    // The rest of the interface is irrelevant for Equalizer
    virtual void receive_notification(const notification &) {}
    virtual bool supports_stereo() { return false; }
    virtual void open(video_frame_format, int, int, float, int, const video_output_state&, unsigned int, int, int) {}
    virtual void activate() {}
    virtual void process_events() {}
    virtual void close() {}
};

/*
 * eq_init_data
 */

class eq_init_data : public eq::net::Object
{
public:
    uint32_t frame_data_id;
    player_init_data init_data;

    eq_init_data() : frame_data_id(EQ_ID_INVALID), init_data()
    {
    }

    ~eq_init_data()
    {
        frame_data_id = EQ_ID_INVALID;
    }

protected:
    virtual void getInstanceData(eq::net::DataOStream &os)
    {
        std::ostringstream oss;
        s11n::save(oss, frame_data_id);
        s11n::save(oss, static_cast<int>(init_data.log_level));
        s11n::save(oss, init_data.filenames);
        s11n::save(oss, static_cast<int>(init_data.input_mode));
        s11n::save(oss, static_cast<int>(init_data.video_mode));
        s11n::save(oss, init_data.video_state.contrast);
        s11n::save(oss, init_data.video_state.brightness);
        s11n::save(oss, init_data.video_state.hue);
        s11n::save(oss, init_data.video_state.saturation);
        s11n::save(oss, init_data.video_state.fullscreen);
        s11n::save(oss, init_data.video_state.swap_eyes);
        s11n::save(oss, init_data.video_flags);
        os << oss.str();
    }

    virtual void applyInstanceData(eq::net::DataIStream &is)
    {
        int x;
        std::string s;
        is >> s;
        std::istringstream iss(s);
        s11n::load(iss, frame_data_id);
        s11n::load(iss, x);
        init_data.log_level = static_cast<msg::level_t>(x);
        s11n::load(iss, init_data.filenames);
        s11n::load(iss, x);
        init_data.input_mode = static_cast<enum input::mode>(x);
        s11n::load(iss, x);
        init_data.video_mode = static_cast<enum video_output::mode>(x);
        s11n::load(iss, init_data.video_state.contrast);
        s11n::load(iss, init_data.video_state.brightness);
        s11n::load(iss, init_data.video_state.hue);
        s11n::load(iss, init_data.video_state.saturation);
        s11n::load(iss, init_data.video_state.fullscreen);
        s11n::load(iss, init_data.video_state.swap_eyes);
        s11n::load(iss, init_data.video_flags);
    }
};

/*
 * eq_frame_data
 */

class eq_frame_data : public eq::net::Object
{
public:
    video_output_state video_state;
    int64_t seek_request;
    bool prep_frame;
    bool drop_frame;
    bool display_frame;

public:
    eq_frame_data()
        : video_state(), seek_request(0),
        prep_frame(false), drop_frame(false), display_frame(false)
    {
    }

protected:
    virtual ChangeType getChangeType() const
    {
        return eq::net::Object::INSTANCE;
    }

    virtual void getInstanceData(eq::net::DataOStream &os)
    {
        std::ostringstream oss;
        s11n::save(oss, video_state.contrast);
        s11n::save(oss, video_state.brightness);
        s11n::save(oss, video_state.hue);
        s11n::save(oss, video_state.saturation);
        s11n::save(oss, video_state.fullscreen);
        s11n::save(oss, video_state.swap_eyes);
        s11n::save(oss, seek_request);
        s11n::save(oss, prep_frame);
        s11n::save(oss, drop_frame);
        s11n::save(oss, display_frame);
        os << oss.str();
    }

    virtual void applyInstanceData(eq::net::DataIStream &is)
    {
        std::string s;
        is >> s;
        std::istringstream iss(s);
        s11n::load(iss, video_state.contrast);
        s11n::load(iss, video_state.brightness);
        s11n::load(iss, video_state.hue);
        s11n::load(iss, video_state.saturation);
        s11n::load(iss, video_state.fullscreen);
        s11n::load(iss, video_state.swap_eyes);
        s11n::load(iss, seek_request);
        s11n::load(iss, prep_frame);
        s11n::load(iss, drop_frame);
        s11n::load(iss, display_frame);
    }
};

/*
 * eq_config
 */

class eq_config : public eq::Config
{
private:
    bool _is_master_config;
    eq_init_data _eq_init_data;               // Master eq_init_data instance
    eq_frame_data _eq_frame_data;             // Master eq_frame_data instance
    player_eq_node _player;

public:
    int src_width, src_height;
    float src_aspect_ratio;
    video_frame_format src_preferred_frame_format;

public:
    eq_config(eq::ServerPtr parent)
        : eq::Config(parent), _is_master_config(false), _eq_init_data(), _eq_frame_data(), _player(),
        src_width(-1), src_height(-1), src_aspect_ratio(0.0f), src_preferred_frame_format(yuv420p)
    {
    }

    bool is_master_config()
    {
        return _is_master_config;
    }

    bool init(const player_init_data &init_data)
    {
        msg::set_level(init_data.log_level);
        msg::dbg(HERE);
        // If this function is called, then this is the master config
        _is_master_config = true;
        // Initialize master instances
        _eq_init_data.init_data = init_data;
        _eq_frame_data.video_state = _eq_init_data.init_data.video_state;
        // Register master instances
        registerObject(&_eq_frame_data);
        _eq_init_data.frame_data_id = _eq_frame_data.getID();
        registerObject(&_eq_init_data);
        // Initialize player
        _player.eq_make_master();
        if (!_player.eq_init(init_data, &src_width, &src_height, &src_aspect_ratio, &src_preferred_frame_format))
        {
            return false;
        }
        msg::dbg(HERE);
        return eq::Config::init(_eq_init_data.getID());
    }

    void prep_frame(video_output *vo)
    {
        _player.eq_prep_frame(vo);
    }

    virtual bool exit()
    {
        msg::dbg(HERE);
        bool ret = eq::Config::exit();
        // Deregister master instances
        deregisterObject(&_eq_init_data);
        deregisterObject(&_eq_frame_data);
        _eq_init_data.frame_data_id = EQ_ID_INVALID;
        msg::dbg(HERE);
        return ret;
    }

    virtual uint32_t startFrame()
    {
        msg::dbg(HERE);
        bool more_steps, prep_frame, drop_frame, display_frame;
        _player.eq_run_step(&more_steps, &prep_frame, &drop_frame, &display_frame);
        if (!more_steps)
        {
            this->exit();
        }
        if (prep_frame)
        {
            _player.eq_get_frame();
        }
        else if (drop_frame)
        {
            _player.eq_release_frame();
        }
        _eq_frame_data.prep_frame = prep_frame;
        _eq_frame_data.drop_frame = drop_frame;
        _eq_frame_data.display_frame = display_frame;

        // Update frame data
        const uint32_t version = _eq_frame_data.commit();
        // Start this frame with the committed frame data
        msg::dbg(HERE);
        return eq::Config::startFrame(version);
    }

    virtual bool handleEvent(const eq::ConfigEvent *event);
};

bool eq_config::handleEvent(const eq::ConfigEvent *event)
{
    if (eq::Config::handleEvent(event))
    {
        return true;
    }

    /* ... */

    return false;
}

/*
 * eq_node
 */

class eq_node : public eq::Node
{
private:
    bool _is_app_node;
    player_eq_node _player;

public:
    eq_init_data init_data;
    eq_frame_data frame_data;
    int src_width, src_height;
    float src_aspect_ratio;
    video_frame_format src_preferred_frame_format;

    eq_node(eq::Config *parent)
        : eq::Node(parent), _is_app_node(false),
        _player(), init_data(), frame_data(),
        src_width(-1), src_height(-1), src_aspect_ratio(-1.0f), src_preferred_frame_format(yuv420p)
    {
    }

protected:
    virtual bool configInit(const uint32_t initID)
    {
        if (!eq::Node::configInit(initID))
        {
            return false;
        }
        // Map our InitData instance to the master instance
        eq_config *config = static_cast<eq_config *>(getConfig());
        if (!config->mapObject(&init_data, initID))
        {
            return false;
        }
        // Is this the application node?
        if (config->is_master_config())
        {
            _is_app_node = true;
        }
        // Map our FrameData instance to the master instance
        if (!config->mapObject(&frame_data, init_data.frame_data_id))
        {
            return false;
        }

        // Create decoders and input
        msg::set_level(init_data.init_data.log_level);
        msg::dbg(HERE);
        if (!_is_app_node)
        {
            if (!_player.eq_init(init_data.init_data, &src_width, &src_height, &src_aspect_ratio, &src_preferred_frame_format))
            {
                return false;
            }
        }
        else
        {
            src_width = config->src_width;
            src_height = config->src_height;
            src_aspect_ratio = config->src_aspect_ratio;
            src_preferred_frame_format = config->src_preferred_frame_format;
        }
        msg::dbg(HERE);
        return true;
    }

    virtual bool configExit()
    {
        msg::dbg(HERE);
        eq::Config *config = getConfig();
        // Unmap our FrameData instance
        config->unmapObject(&frame_data);
        // Unmap our InitData instance
        config->unmapObject(&init_data);
        msg::dbg(HERE);
        return eq::Node::configExit();
    }

    virtual void frameStart(const uint32_t frameID, const uint32_t frameNumber)
    {
        msg::dbg(HERE);
        // Update our FrameData instance from the master instance
        frame_data.sync(frameID);

        if (!_is_app_node)
        {
            if (frame_data.prep_frame)
            {
                _player.eq_get_frame();
            }
            else if (frame_data.drop_frame)
            {
                _player.eq_release_frame();
            }
        }

        startFrame(frameNumber);
        msg::dbg(HERE);
    }

    virtual void frameFinish(const uint32_t, const uint32_t frameNumber)
    {
        msg::dbg(HERE);
        if (!_is_app_node)
        {
            if (frame_data.prep_frame)
            {
                _player.eq_release_frame();
            }
        }
        releaseFrame(frameNumber);
        msg::dbg(HERE);
    }

public:
    void prep_frame(video_output *vo, video_frame_format fmt)
    {
        msg::dbg(HERE);
        if (fmt != src_preferred_frame_format)
        {
            msg::err("cannot provide video in requested frame format");
            abort();
        }
        if (_is_app_node)
        {
            _player.eq_prep_frame(vo);
        }
        else
        {
            eq_config *config = static_cast<eq_config *>(getConfig());
            config->prep_frame(vo);
        }
        msg::dbg(HERE);
    }
};

/*
 * eq_pipe
 */

class eq_pipe : public eq::Pipe
{
public:
    eq_pipe(eq::Node *parent) : eq::Pipe(parent)
    {
    }
};

/*
 * eq_window
 */

class eq_window : public eq::Window
{
private:
    video_output_opengl_eq_window _video_output;

public:
    eq_window(eq::Pipe *parent) : eq::Window(parent), _video_output()
    {
    }

    void display(video_output::mode mode)
    {
        _video_output.eq_display(mode, -1.0f, -1.0f, 1.0f, 1.0f);
    }

protected:
    virtual bool configInitGL(const uint32_t initID)
    {
        msg::dbg(HERE);
        if (!eq::Window::configInitGL(initID))
        {
            return false;
        }
        eq_node *node = static_cast<eq_node *>(getNode());

        // Disable some things that Equalizer seems to enable for some reason.
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_LIGHTING);

        bool have_pixel_buffer_object = glewContextIsSupported(
                const_cast<GLEWContext *>(glewGetContext()), "GL_ARB_pixel_buffer_object");
        bool have_texture_non_power_of_two = glewContextIsSupported(
                const_cast<GLEWContext *>(glewGetContext()), "GL_ARB_texture_non_power_of_two");
        bool have_fragment_shader = glewContextIsSupported(
                const_cast<GLEWContext *>(glewGetContext()), "GL_ARB_fragment_shader");

        _video_output.eq_initialize(node->src_width, node->src_height, node->src_aspect_ratio, node->src_preferred_frame_format,
                have_pixel_buffer_object, have_texture_non_power_of_two, have_fragment_shader);

        msg::dbg(HERE);
        return true;
    }

    virtual bool configExitGL()
    {
        msg::dbg(HERE);
        _video_output.eq_deinitialize();
        msg::dbg(HERE);
        return eq::Window::configExitGL();
    }

    virtual void frameStart(const uint32_t, const uint32_t frameNumber)
    {
        msg::dbg(HERE);
        eq_node *node = static_cast<eq_node *>(getNode());
        if (node->frame_data.prep_frame)
        {
            node->prep_frame(&_video_output, _video_output.frame_format());
        }
        else if (node->frame_data.display_frame)
        {
            _video_output.eq_swap_tex_set();
        }
        startFrame(frameNumber);
        msg::dbg(HERE);
    }

    virtual void frameFinish(const uint32_t, const uint32_t frameNumber)
    {
        msg::dbg(HERE);
        releaseFrame(frameNumber);
        msg::dbg(HERE);
    }
};

/*
 * eq_channel
 */

class eq_channel : public eq::Channel
{
private:

public:
    eq_channel(eq::Window *parent) : eq::Channel(parent)
    {
    }

protected:
    virtual void frameDraw(const uint32_t frameID)
    {
        msg::dbg(HERE);
        // Let Equalizer initialize some stuff
        eq::Channel::frameDraw(frameID);

        // Get the current state
        //eq_node *node = static_cast<eq_node *>(getNode());
        eq_window *window = static_cast<eq_window *>(getWindow());

        eq::PixelViewport eq_viewport = getPixelViewport();
        eq::Frustumf eq_frustum = getFrustum();
        eq::Matrix4f eq_head_transform = getHeadTransform();

        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        window->display(getEye() == eq::EYE_RIGHT ? video_output::mono_right : video_output::mono_left);
        msg::dbg(HERE);
    }
};

/*
 * eq_node_factory
 */

class eq_node_factory : public eq::NodeFactory
{
public:
    virtual eq::Config *createConfig(eq::ServerPtr parent)
    {
        return new eq_config(parent);
    }

    virtual eq::Node *createNode(eq::Config *parent)
    {
        return new eq_node(parent);
    }

    virtual eq::Pipe *createPipe(eq::Node *parent)
    {
        return new eq_pipe(parent);
    }

    virtual eq::Window *createWindow(eq::Pipe *parent)
    {
        return new eq_window(parent);
    }

    virtual eq::Channel *createChannel(eq::Window *parent)
    {
        return new eq_channel(parent);
    }
};

/*
 * player_equalizer
 */

player_equalizer::player_equalizer(int *argc, char *argv[])
    : player(player::slave)
{
    /* Initialize Equalizer */
    eq_node_factory node_factory;
    if (!eq::init(*argc, argv, &node_factory))
    {
        throw exc("Equalizer initialization failed");
    }

    /* Get a configuration */
    eq_config *config = static_cast<eq_config *>(eq::getConfig(*argc, argv));
    // The following code is only executed on the application node because
    // eq::getConfig() does not return on other nodes.
    _config = static_cast<void *>(config);
    if (!_config)
    {
        throw exc("Cannot get Equalizer configuration");
    }
}

player_equalizer::~player_equalizer()
{
}

void player_equalizer::open(const player_init_data &init_data)
{
    eq_config *config = static_cast<eq_config *>(_config);
    if (!config->init(init_data))
    {
        throw exc("Equalizer configuration initialization failed");
    }
}

void player_equalizer::run()
{
    eq_config *config = static_cast<eq_config *>(_config);
    while (config->isRunning())
    {
        config->startFrame();
        config->finishFrame();
    }
    config->exit();
    eq::releaseConfig(config);
    eq::exit();
}

void player_equalizer::close()
{
}