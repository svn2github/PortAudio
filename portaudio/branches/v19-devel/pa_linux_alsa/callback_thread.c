
#include <sys/poll.h>
#include <limits.h>

#include <alsa/asoundlib.h>

#include "pa_linux_alsa.h"

#define MIN(x,y) ( (x) < (y) ? (x) : (y) )

static int wait( PaAlsaStream *stream )
{
    int need_capture;
    int need_playback;
    int capture_avail = INT_MAX;
    int playback_avail = INT_MAX;
    int common_avail;

    if( stream->pcm_capture )
        need_capture = 1;
    else
        need_capture = 0;

    if( stream->pcm_playback )
        need_playback = 1;
    else
        need_playback = 0;

    while( need_capture || need_playback )
    {
        int playback_pfd_offset;
        int total_fds = 0;

        /* get the fds, packing all applicable fds into a single array,
         * so we can check them all with a single poll() call */

        if( need_capture )
        {
            snd_pcm_poll_descriptors( stream->pcm_capture, stream->pfds,
                                      stream->capture_nfds );
            total_fds += stream->capture_nfds;
        }

        if( need_playback )
        {
            playback_pfd_offset = total_fds;
            snd_pcm_poll_descriptors( stream->pcm_playback,
                                      stream->pfds + playback_pfd_offset,
                                      stream->playback_nfds );
            total_fds += stream->playback_nfds;
        }

        /* now poll on the combination of playback and capture fds.
         * TODO: handle interrupt and/or failure */
        poll( stream->pfds, total_fds, 1000 );

        /* check the return status of our pfds */
        if( need_capture )
        {
            short revents;
            snd_pcm_poll_descriptors_revents( stream->pcm_capture, stream->pfds,
                                              stream->capture_nfds, &revents );
            if( revents == POLLIN )
                need_capture = 0;
        }

        if( need_playback )
        {
            short revents;
            snd_pcm_poll_descriptors_revents( stream->pcm_playback,
                                              stream->pfds + playback_pfd_offset,
                                              stream->playback_nfds, &revents );
            if( revents == POLLOUT )
                need_playback = 0;
        }
    }

    /* we have now established that there are buffers ready to be
     * operated on.  Now determine how many frames are available. */
    if( stream->pcm_capture )
        capture_avail = snd_pcm_avail_update( stream->pcm_capture );

    if( stream->pcm_playback )
        playback_avail = snd_pcm_avail_update( stream->pcm_playback );

    common_avail = MIN(capture_avail, playback_avail);
    common_avail -= common_avail % stream->frames_per_period;

    return common_avail;
}

static int setup_buffers( PaAlsaStream *stream, int frames_avail )
{
    int i;
    int capture_frames_avail = INT_MAX;
    int playback_frames_avail = INT_MAX;
    int common_frames_avail;

    if( stream->pcm_capture )
    {
        const snd_pcm_channel_area_t *capture_areas;
        const snd_pcm_channel_area_t *area;
        snd_pcm_uframes_t frames = frames_avail;

        /* I do not understand this code fragment yet, it is copied out of the
         * alsa-devel archives... */
        snd_pcm_mmap_begin( stream->pcm_capture, &capture_areas,
                            &stream->capture_offset, &frames);

        if( stream->capture_interleaved )
        {
            void *interleaved_capture_buffer;
            area = &capture_areas[0];
            interleaved_capture_buffer = area->addr +
                              (area->first + area->step * stream->capture_offset) / 8;
            PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor,
                                                0, /* starting at channel 0 */
                                                interleaved_capture_buffer,
                                                0  /* default numInputChannels */
                                              );
        }
        else
        {
            /* noninterleaved */
            for( i = 0; i < stream->capture_channels; i++ )
            {
                void **noninterleaved_capture_buffers;
                area = &capture_areas[i];
                noninterleaved_capture_buffers[i] = area->addr +
                                  (area->first + area->step * stream->capture_offset) / 8;
                PaUtil_SetNonInterleavedInputChannel( &stream->bufferProcessor,
                                                      i,
                                                      noninterleaved_capture_buffers[i]);
            }
        }

        capture_frames_avail = frames;
    }

    if( stream->pcm_playback )
    {
        const snd_pcm_channel_area_t *playback_areas;
        const snd_pcm_channel_area_t *area;
        snd_pcm_uframes_t frames = frames_avail;

        /* I do not understand this code fragment yet, it is copied out of the
         * alsa-devel archives... */
        snd_pcm_mmap_begin( stream->pcm_playback, &playback_areas, 
                            &stream->playback_offset, &frames);

        if( stream->playback_interleaved )
        {
            void *interleaved_playback_buffer;
            area = &playback_areas[0];
            interleaved_playback_buffer = area->addr +
                              (area->first + area->step * stream->playback_offset) / 8;
            PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor,
                                                 0, /* starting at channel 0 */
                                                 interleaved_playback_buffer,
                                                 0  /* default numInputChannels */
                                               );
        }
        else
        {
            /* noninterleaved */
            for( i = 0; i < stream->playback_channels; i++ )
            {
                void **noninterleaved_playback_buffers;
                area = &playback_areas[i];
                noninterleaved_playback_buffers[i] = area->addr +
                                  (area->first + area->step * stream->playback_offset) / 8;
                PaUtil_SetNonInterleavedOutputChannel( &stream->bufferProcessor,
                                                      i,
                                                      noninterleaved_playback_buffers[i]);
            }
        }

        playback_frames_avail = frames;
    }


    common_frames_avail = MIN(capture_frames_avail, playback_frames_avail);
    common_frames_avail -= common_frames_avail % stream->frames_per_period;
    //printf( "%d capture frames available\n", capture_frames_avail );
    //printf( "%d frames playback available\n", playback_frames_avail );
    //printf( "%d frames available\n", common_frames_avail );

    if( stream->pcm_capture )
        PaUtil_SetInputFrameCount( &stream->bufferProcessor, common_frames_avail );

    if( stream->pcm_playback )
        PaUtil_SetOutputFrameCount( &stream->bufferProcessor, common_frames_avail );

    return common_frames_avail;
}

void *CallbackThread( void *userData )
{
    PaAlsaStream *stream = (PaAlsaStream*)userData;

    while(1)
    {
        int frames_avail;
        int frames_got;

        PaTimestamp outTime = 0; /* IMPLEMENT ME */
        int callbackResult;
        int framesProcessed;



        /*
            IMPLEMENT ME:
                - generate timing information
                - handle buffer slips
        */

        /*
            If you need to byte swap inputBuffer, you can do it here using
            routines in pa_byteswappers.h
        */

        /*
            depending on whether the host buffers are interleaved, non-interleaved
            or a mixture, you will want to call PaUtil_ProcessInterleavedBuffers(),
            PaUtil_ProcessNonInterleavedBuffers() or PaUtil_ProcessBuffers() here.
        */

        frames_avail = wait( stream );
        printf( "%d frames available\n", frames_avail );

        /* Now we know the soundcard is ready to produce/receive at least
         * one period.  We just need to get the buffers for the client
         * to read/write. */
        PaUtil_BeginBufferProcessing( &stream->bufferProcessor, outTime );

        frames_got = setup_buffers( stream, frames_avail );

        if( frames_avail == frames_got )
            printf("good, they were both %d\n", frames_avail );
        else
            printf("damn, they were different: avail: %d, got: %d\n", frames_avail, frames_got );

        /* this calls the callback */

        PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );

        framesProcessed = PaUtil_EndBufferProcessing( &stream->bufferProcessor,
                                                      &callbackResult );

        PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesProcessed );

        /* inform ALSA how many frames we wrote */

        if( stream->pcm_capture )
            snd_pcm_mmap_commit( stream->pcm_capture, stream->capture_offset, frames_avail );

        if( stream->pcm_playback )
            snd_pcm_mmap_commit( stream->pcm_playback, stream->playback_offset, frames_avail );


        /*
            If you need to byte swap outputBuffer, you can do it here using
            routines in pa_byteswappers.h
        */

        if( callbackResult == paContinue )
        {
            /* nothing special to do */
        }
        else if( callbackResult == paAbort )
        {
            /* IMPLEMENT ME - finish playback immediately  */
        }
        else
        {
            /* User callback has asked us to stop with paComplete or other non-zero value */

            /* IMPLEMENT ME - finish playback once currently queued audio has completed  */
        }

        /* if the main thread has requested that we stop, do so now */
        pthread_testcancel();
    }
}
