#ifndef PLAYER_EQUALIZER_H
#define PLAYER_EQUALIZER_H

#include "player.h"


class player_equalizer : public player
{
private:
    void *_config;

public:
    player_equalizer(int *argc, char *argv[]);
    ~player_equalizer();

    virtual void open(const player_init_data &init_data);

    virtual void run();

    virtual void close();
};

#endif