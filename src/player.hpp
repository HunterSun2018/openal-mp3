#pragma once
#include <memory>
#include <vector>
#include <string_view>

class PlayStream
{
public:
    virtual ~PlayStream(){};

    virtual void OpenStream() = 0;

    virtual void Play(std::string_view mp3_file_name) = 0;

    virtual void Stop() = 0;

    virtual void Pasue() = 0;
};

using play_stream_ptr = std::shared_ptr<PlayStream>;

play_stream_ptr create_play_stream();