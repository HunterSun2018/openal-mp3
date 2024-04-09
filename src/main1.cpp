#include <AL/al.h>
#include <AL/alc.h>
#include <stdio.h>
#define NUM_BUFFERS 4
#include <mpg123.h>
#include <unistd.h>

int main1(int argc, char *argv[])
{
    ALuint g_Buffers[NUM_BUFFERS];
    ALuint uiSource;
    ALuint uiBuffer;
    ALCdevice *pDevice = NULL;
    ALCcontext *pContext = NULL;
    ALboolean g_bEAX;
    ALenum error, eBufferFormat;
    ALint iDataSize, iFrequency;
    ALuint ulFormat;
    int iLoop, iBuffersProcessed, iTotalBuffersProcessed;
    unsigned long ulDataSize = 0;
    unsigned long ulFrequency = 0;
    unsigned long ulBufferSize;
    unsigned long ulBytesWritten;
    long lRate;
    int iEncoding, iChannels;
    int encoding;
    void *pData = NULL;
    void *pwfex;

    int iState, iQueuedBuffers;
    mpg123_handle *mpg123;
    int iMpg123_error;

    // init mpg123 library
    if (MPG123_OK != (iMpg123_error = mpg123_init()))
    {
        printf("failed to init mpg123\n");
        return -1;
    }

    // open a default mpg123 decoder
    mpg123 = mpg123_new(mpg123_decoders()[0], &iMpg123_error);

    // open a openal default device
    pDevice = alcOpenDevice(NULL); // select the perfered device

    if (pDevice)
    {
        pContext = alcCreateContext(pDevice, NULL);
        alcMakeContextCurrent(pContext);
    }
    else
    {
        printf("failed to get a openal decice\n");
        return -2;
    }

    g_bEAX = alIsExtensionPresent("EAX2.0");

    // Generate openal  Buffers
    alGetError(); // clear all error code

    alGenBuffers(NUM_BUFFERS, g_Buffers);

    if ((error = alGetError()) != AL_NO_ERROR)
    {
        fprintf(stderr, "alGenBuffers :%d", error);
    }

    alGenSources(1, &uiSource);

    // try to open a mp3 file, modify to your own mp3 files
    if (MPG123_OK != (iMpg123_error = mpg123_open(mpg123, argv[1])))
    {
        fprintf(stderr, "error in open mp3 file\n");
        return -1;
    }
    // get mp3 format infomation
    mpg123_getformat(mpg123, &lRate, &iChannels, &iEncoding);

    // there only parse stereo mp3 file
    if (iChannels == 2)
    {
        ulFormat = alGetEnumValue("AL_FORMAT_STEREO16");
        ulBufferSize = lRate;               // set buffer to 250ms
        ulBufferSize -= (ulBufferSize % 4); // set pcm Block align
        ulFrequency = lRate;                // set pcm sample rate
    }
    else
    {
        printf("channels info%i\n", iChannels);
        return -3;
    }

    pData = malloc(ulBufferSize);
    // feed data to openal buffer
    for (iLoop = 0; iLoop < 4; iLoop++)
    {
        mpg123_read(mpg123, (char *)pData, ulBufferSize, &ulBytesWritten);
        alBufferData(g_Buffers[iLoop], ulFormat, pData, ulBytesWritten, ulFrequency);
        error = alGetError();
        alSourceQueueBuffers(uiSource, 1, &g_Buffers[iLoop]);
    }
    alSourcePlay(uiSource);
    iTotalBuffersProcessed = 0;
    printf("playing\n");
    // wait for key press
    while (true)
    {
        usleep(10000);
        iBuffersProcessed = 0;
        alGetSourcei(uiSource, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

        iTotalBuffersProcessed += iBuffersProcessed;
        printf("Buffers total Processed %d\r", iTotalBuffersProcessed);
        while (iBuffersProcessed)
        {
            uiBuffer = 0;
            alSourceUnqueueBuffers(uiSource, 1, &uiBuffer);

            mpg123_read(mpg123, (char *)pData, ulBufferSize, &ulBytesWritten);
            if (ulBytesWritten)
            {
                alBufferData(uiBuffer, ulFormat, pData, ulBytesWritten, ulFrequency);
                alSourceQueueBuffers(uiSource, 1, &uiBuffer);
            }
            iBuffersProcessed--;
        }
        alGetSourcei(uiSource, AL_SOURCE_STATE, &iState);
        if (iState != AL_PLAYING)
        {
            alGetSourcei(uiSource, AL_BUFFERS_QUEUED, &iQueuedBuffers);

            if (iQueuedBuffers)
            {
                alSourcePlay(uiSource); // buffers have data, play it
            }
            else
            {
                // there is no data any more
                break;
            }
        }
    }
    // close mpg123
    mpg123_close(mpg123);
    // stop the source and clear the queue
    alSourceStop(uiSource);
    alSourcei(uiSource, AL_BUFFER, 0);

    free(pData);
    pData = NULL;

    return 0;
}
