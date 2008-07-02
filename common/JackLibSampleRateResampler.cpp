/*
Copyright (C) 2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "JackLibSampleRateResampler.h"

namespace Jack
{

JackLibSampleRateResampler::JackLibSampleRateResampler()
    :JackResampler(),fRatio(1)
{
    jack_log("JackLibSampleRateResampler::JackLibSampleRateResampler");
    int error;
    fResampler = src_new(SRC_LINEAR, 1, &error);
    if (error != 0) 
        jack_error("JackLibSampleRateResampler::JackLibSampleRateResampler err = %s", src_strerror(error));
}

JackLibSampleRateResampler::~JackLibSampleRateResampler()
{
    src_delete(fResampler);
}

int JackLibSampleRateResampler::ReadResample(float* buffer, unsigned int frames)
{
    jack_ringbuffer_data_t ring_buffer_data[2];
    SRC_DATA src_data;
    unsigned int frames_to_write = frames;
    unsigned int written_frames = 0;
    int res;
    
    jack_ringbuffer_get_read_vector(fRingBuffer, ring_buffer_data);
    unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(float);
    jack_log("OUPUT available = %ld", available_frames);
      
    for (int j = 0; j < 2; j++) {
    
        if (ring_buffer_data[j].len > 0) {
            
            src_data.data_in = (float*)ring_buffer_data[j].buf;
            src_data.data_out = &buffer[written_frames];
            src_data.input_frames = ring_buffer_data[j].len / sizeof(float);
            src_data.output_frames = frames_to_write;
            src_data.end_of_input = 0;
            src_data.src_ratio = fRatio;
             
            res = src_process(fResampler, &src_data);
            if (res != 0)
                jack_error("JackLibSampleRateResampler::ReadResample err = %s", src_strerror(res));
                
            frames_to_write -= src_data.output_frames_gen;
            written_frames += src_data.output_frames_gen;
            
            if ((src_data.input_frames_used == 0 || src_data.output_frames_gen == 0) && j == 0) {
                jack_error("OUTPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            }
            
            jack_log("OUTPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            jack_ringbuffer_read_advance(fRingBuffer, src_data.input_frames_used * sizeof(float));
        }
    }
    
    if (written_frames < frames) {
        jack_error("OUPUT available = %ld", available_frames);
        jack_error("JackLibSampleRateResampler::ReadResample error written_frames = %ld", written_frames);
    }
        
    return written_frames;
}

int JackLibSampleRateResampler::WriteResample(float* buffer, unsigned int frames)
{
    jack_ringbuffer_data_t ring_buffer_data[2];
    SRC_DATA src_data;
    unsigned int frames_to_read = frames;
    unsigned int read_frames = 0;
    int res;
    
    jack_ringbuffer_get_write_vector(fRingBuffer, ring_buffer_data);
    unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(float);
    jack_log("INPUT available = %ld", available_frames);
    
    for (int j = 0; j < 2; j++) {
            
        if (ring_buffer_data[j].len > 0) {
        
            src_data.data_in = &buffer[read_frames];
            src_data.data_out = (float*)ring_buffer_data[j].buf;
            src_data.input_frames = frames_to_read;
            src_data.output_frames = (ring_buffer_data[j].len / sizeof(float));
            src_data.end_of_input = 0;
            src_data.src_ratio = fRatio;
         
            res = src_process(fResampler, &src_data);
            if (res != 0)
                jack_error("JackLibSampleRateResampler::ReadResample err = %s", src_strerror(res));
                
            frames_to_read -= src_data.input_frames_used;
            read_frames += src_data.input_frames_used;
            
            if ((src_data.input_frames_used == 0 || src_data.output_frames_gen == 0) && j == 0) {
                jack_error("INPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            }
        
            jack_log("INPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            jack_ringbuffer_write_advance(fRingBuffer, src_data.output_frames_gen * sizeof(float));
        }
    }
    
    if (read_frames < frames) {
        jack_error("INPUT available = %ld", available_frames);
        jack_error("JackLibSampleRateResampler::ReadResample error read_frames = %ld", read_frames);
    }
        
    return read_frames;
}

}
