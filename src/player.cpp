#include <AL/al.h>
#include <AL/alc.h>
#include <mpg123.h>
#include <format>
#include <fstream>
#include <cstring>

#include "player.hpp"
#include <unistd.h>

using namespace std;

class PlayerImp : public PlayStream
{
private:
    /* data */

    void init();
    bool updataQueueBuffer();
    void mpgClose();

    static const int NUM_BUFFERS = 4;
    ALuint _buffers[NUM_BUFFERS];

    mpg123_handle *mh;
    ALCdevice *pDevice;
    ALCcontext *pContext;
    ALuint outSourceID;
    ALboolean _bEAX;
    long lRate;

public:
    PlayerImp(/* args */);

    virtual ~PlayerImp();

    virtual void OpenStream() override;

    virtual void Play(std::string_view mp3_file_name) override;

    virtual void Stop() override;

    virtual void Pasue() override;
};

play_stream_ptr create_play_stream()
{
    return std::make_shared<PlayerImp>();
}

PlayerImp::PlayerImp(/* args */)
{
    init();
}

PlayerImp::~PlayerImp()
{
    // Exit
    auto Context = alcGetCurrentContext();
    auto Device = alcGetContextsDevice(Context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(Context);
    alcCloseDevice(Device);
}

static void list_audio_devices(const ALCchar *devices)
{
    const ALCchar *device = devices, *next = devices + 1;
    size_t len = 0;

    // fprintf(stdout, "Devices list:\n");
    // fprintf(stdout, "----------\n");
    while (device && *device != '\0' && next && *next != '\0')
    {
        fprintf(stdout, "%s\n", device);
        len = strlen(device);
        device += (len + 1);
        next += (len + 2);
    }
    // fprintf(stdout, "----------\n");
}

void check_and_throw_al_error(string_view fun)
{
    ALenum err;
    if ((err = alGetError()) != AL_NO_ERROR)
        throw runtime_error(format("Failed in {} : {:X}", fun, err));
}

void PlayerImp::init()
{
    int err;

    list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

    auto dev = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

    pDevice = alcOpenDevice(nullptr);
    if (pDevice)
    {
        pContext = alcCreateContext(pDevice, NULL);
        alcMakeContextCurrent(pContext);
    }

    _bEAX = alIsExtensionPresent("EAX2.0");

    // clear error code
    alGetError(); 

    // Generate Buffers
    alGenBuffers(NUM_BUFFERS, _buffers);
    if ((err = alGetError()) != AL_NO_ERROR)
        throw runtime_error(format("Failed in alGenBuffers : {}", err));

    alGenSources(1, &outSourceID);
    check_and_throw_al_error("alGenSources");

    // alSourcei(outSourceID, AL_LOOPING, AL_FALSE);
    // alSourcef(outSourceID, AL_SOURCE_TYPE, AL_STREAMING);

    mpg123_init();
    // mh = mpg123_new(NULL, &err);
    mh = mpg123_new(mpg123_decoders()[0], &err);
    // err = mpg123_param2(mh, MPG123_VERBOSE, 2, 0);
    // err = mpg123_open_feed(mh);
}

void PlayerImp::Play(std::string_view mp3_file_name)
{
    ifstream ifs(mp3_file_name.data(), ios::binary);

    if (ifs.fail())
        throw runtime_error(format("cannot open file {}", mp3_file_name));

    istreambuf_iterator<char> isb(ifs), empty;
    vector<uint8_t> data1(isb, empty);

    ifs.seekg(0, std::ifstream::end);
    streampos size = ifs.tellg();
    ifs.seekg(0);

    vector<uint8_t> data(size);

    ifs.read((char *)data.data(), size);

    // updataQueueBuffer();
    // ALuint bufferID = 0;
    // alGenBuffers(1, &bufferID);
    size_t done = 0;
    // size_t bufferSize = 1'638'400 * 256;

    ALuint ulFormat = alGetEnumValue("AL_FORMAT_STEREO16");
    int error = 0;

    if (mpg123_open(mh, mp3_file_name.data()))
        throw runtime_error(format("cannot open file {}", mp3_file_name));

    long rate;
    int channels, coding;

    if (mpg123_getformat(mh, &rate, &channels, &coding))
        throw runtime_error(format("cannot get format for file {}", mp3_file_name));

    ulFormat = alGetEnumValue("AL_FORMAT_STEREO16");
    size_t bufferSize = rate;       // set buffer to 250ms
    bufferSize -= (bufferSize % 4); // set pcm Block align
    size_t ulFrequency = rate;      // set pcm sample rate

    vector<uint8_t> buffer(bufferSize);

    // feed data to openal buffer
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        size_t bytesRead = 0;
        error = mpg123_read(mh, (void *)buffer.data(), buffer.size(), &bytesRead);
        alBufferData(_buffers[i], ulFormat, buffer.data(), bytesRead, ulFrequency);
        check_and_throw_al_error("alBufferData");
        alSourceQueueBuffers(outSourceID, 1, &_buffers[i]);
        error = alGetError();
    }

    alSourcePlay(outSourceID);
    check_and_throw_al_error("alSourcePlay");

    // vector<uint16_t> buffer1(1'638'400);
    // error = mpg123_decode(mh, data1.data(), data1.size(), buffer1.data(), buffer1.size(), &done);
    // if (done)
    // {
    //     alBufferData(outSourceID, ulFormat, buffer.data(), done, 44100);
    //     // alSourceQueueBuffers(outSourceID, 1, &bufferID);
    // }

    while (true)
    {
        ALint state, value, n;

        alGetSourcei(outSourceID, AL_BUFFERS_QUEUED, &value);

        alGetSourcei(outSourceID, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            alSourcePlay(outSourceID);
        }

        while (alGetSourcei(outSourceID, AL_BUFFERS_PROCESSED, &n), n == 0)
        {
            usleep(10000);
        }

        ALuint unque_buffer;
        alSourceUnqueueBuffers(outSourceID, 1, &unque_buffer);

        size_t bytesRead = 0;
        error = mpg123_read(mh, (void *)buffer.data(), buffer.size(), &bytesRead);

        if (bytesRead == 0)
            break;

        alBufferData(unque_buffer, ulFormat, buffer.data(), bytesRead, rate);
        alSourceQueueBuffers(outSourceID, 1, &unque_buffer);

        // alDeleteBuffers(1, &bufferID);
    }

    
}

bool PlayerImp::updataQueueBuffer()
{
    ALint stateVaue;
    int processed, queued;
    alGetSourcei(outSourceID, AL_SOURCE_STATE, &stateVaue);
    if (stateVaue == AL_STOPPED)
    {
        return false;
    }

    alGetSourcei(outSourceID, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(outSourceID, AL_BUFFERS_QUEUED, &queued);
    while (processed--)
    {
        ALuint buff;
        alSourceUnqueueBuffers(outSourceID, 1, &buff);
        alDeleteBuffers(1, &buff);
    }
    return true;
}

void PlayerImp::mpgClose()
{
    alSourceStop(outSourceID);
    alSourcei(outSourceID, AL_BUFFER, 0);
    int err = mpg123_close(mh);
}

void PlayerImp::OpenStream()
{
    int err = mpg123_open_feed(mh);
}

void PlayerImp::Stop()
{
    mpgClose();
    alcCloseDevice(pDevice);
    alcDestroyContext(pContext);
}

void PlayerImp::Pasue()
{
    alSourcePause(outSourceID);
}