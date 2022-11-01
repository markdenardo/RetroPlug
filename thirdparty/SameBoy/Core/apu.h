#ifndef apu_h
#define apu_h
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "defs.h"

#ifdef GB_INTERNAL
/* Speed = 1 / Length (in seconds) */
#define DAC_DECAY_SPEED 20000
#define DAC_ATTACK_SPEED 20000


/* Divides nicely and never overflows with 4 channels and 8 (1-8) volume levels */
#ifdef WIIU
/* Todo: Remove this hack once https://github.com/libretro/RetroArch/issues/6252 is fixed*/
#define MAX_CH_AMP (0xFF0 / 2)
#else
#define MAX_CH_AMP 0xFF0
#endif
#define CH_STEP (MAX_CH_AMP/0xF/8)
#endif



/* APU ticks are 2MHz, triggered by an internal APU clock. */

typedef struct
{
    int16_t left;
    int16_t right;
} GB_sample_t;

typedef struct
{
    double left;
    double right;
} GB_double_sample_t;

typedef enum {
    GB_SQUARE_1,
    GB_SQUARE_2,
    GB_WAVE,
    GB_NOISE,
    GB_N_CHANNELS
} GB_channel_t;

typedef struct
{
    bool locked:1;
    bool clock:1; // Represents FOSY on channel 4
    unsigned padding:6;
} GB_envelope_clock_t;

typedef void (*GB_sample_callback_t)(GB_gameboy_t *gb, GB_sample_t *sample);

typedef struct
{
    bool global_enable;
    uint16_t apu_cycles;

    uint8_t samples[GB_N_CHANNELS];
    bool is_active[GB_N_CHANNELS];

    uint8_t div_divider; // The DIV register ticks the APU at 512Hz, but is then divided
                         // once more to generate 128Hz and 64Hz clocks

    uint8_t lf_div; // The APU runs in 2MHz, but channels 1, 2 and 4 run in 1MHZ so we divide
                    // need to divide the signal.

    uint8_t square_sweep_countdown; // In 128Hz
    uint8_t square_sweep_calculate_countdown; // In 2 MHz
    uint16_t sweep_length_addend;
    uint16_t shadow_sweep_sample_length;
    bool unshifted_sweep;
    bool enable_zombie_calculate_stepping;
    
    uint8_t channel_1_restart_hold;
    uint16_t channel1_completed_addend;
    struct {
        uint16_t pulse_length; // Reloaded from NRX1 (xorred), in 256Hz DIV ticks
        uint8_t current_volume; // Reloaded from NRX2
        uint8_t volume_countdown; // Reloaded from NRX2
        uint8_t current_sample_index;
        bool sample_surpressed;

        uint16_t sample_countdown; // in APU ticks (Reloaded from sample_length, xorred $7FF)
        uint16_t sample_length; // From NRX3, NRX4, in APU ticks
        bool length_enabled; // NRX4
        GB_envelope_clock_t envelope_clock;
        uint8_t delay; // Hack for CGB D/E phantom step due to how sample_countdown is implemented in SameBoy
        bool did_tick;
    } square_channels[2];

    struct {
        bool enable; // NR30
        uint16_t pulse_length; // Reloaded from NR31 (xorred), in 256Hz DIV ticks
        uint8_t shift; // NR32
        uint16_t sample_length; // NR33, NR34, in APU ticks
        bool length_enabled; // NR34

        uint16_t sample_countdown; // in APU ticks (Reloaded from sample_length, xorred $7FF)
        uint8_t current_sample_index;
        uint8_t current_sample_byte; // Current sample byte.
        bool wave_form_just_read;
        bool pulsed;
        uint8_t bugged_read_countdown;
    } wave_channel;

    struct {
        uint16_t pulse_length; // Reloaded from NR41 (xorred), in 256Hz DIV ticks
        uint8_t current_volume; // Reloaded from NR42
        uint8_t volume_countdown; // Reloaded from NR42
        uint16_t lfsr;
        bool narrow;

        uint8_t counter_countdown; // Counts from 0-7 to 0 to tick counter (Scaled from 512KHz to 2MHz)
        uint16_t counter; // A bit from this 14-bit register ticks LFSR
        bool length_enabled; // NR44

        uint8_t alignment; // If (NR43 & 7) != 0, samples are aligned to 512KHz clock instead of
                           // 1MHz. This variable keeps track of the alignment.
        bool current_lfsr_sample;
        int8_t delta;
        bool countdown_reloaded;
        uint8_t dmg_delayed_start;
        GB_envelope_clock_t envelope_clock;
    } noise_channel;

    enum {
        GB_SKIP_DIV_EVENT_INACTIVE,
        GB_SKIP_DIV_EVENT_SKIPPED,
        GB_SKIP_DIV_EVENT_SKIP,
    } skip_div_event:8;
    uint8_t pcm_mask[2]; // For CGB-0 to CGB-C PCM read glitch
} GB_apu_t;

typedef enum {
    GB_HIGHPASS_OFF, // Do not apply any filter, keep DC offset
    GB_HIGHPASS_ACCURATE, // Apply a highpass filter similar to the one used on hardware
    GB_HIGHPASS_REMOVE_DC_OFFSET, // Remove DC Offset without affecting the waveform
    GB_HIGHPASS_MAX
} GB_highpass_mode_t;

typedef enum {
    GB_AUDIO_FORMAT_RAW, // Native endian
    GB_AUDIO_FORMAT_AIFF, // Native endian
    GB_AUDIO_FORMAT_WAV,
} GB_audio_format_t;

typedef struct {
    unsigned sample_rate;

    unsigned sample_cycles; // Counts by sample_rate until it reaches the clock frequency

    // Samples are NOT normalized to MAX_CH_AMP * 4 at this stage!
    unsigned cycles_since_render;
    unsigned last_update[GB_N_CHANNELS];
    GB_sample_t current_sample[GB_N_CHANNELS];
    GB_sample_t summed_samples[GB_N_CHANNELS];
    double dac_discharge[GB_N_CHANNELS];
    bool channel_muted[GB_N_CHANNELS];

    GB_highpass_mode_t highpass_mode;
    double highpass_rate;
    GB_double_sample_t highpass_diff;
    
    GB_sample_callback_t sample_callback;
    
    double interference_volume;
    double interference_highpass;
    
    FILE *output_file;
    GB_audio_format_t output_format;
    int output_error;
    
} GB_apu_output_t;

void GB_set_channel_muted(GB_gameboy_t *gb, GB_channel_t channel, bool muted);
bool GB_is_channel_muted(GB_gameboy_t *gb, GB_channel_t channel);
void GB_set_sample_rate(GB_gameboy_t *gb, unsigned sample_rate);
unsigned GB_get_sample_rate(GB_gameboy_t *gb);
void GB_set_sample_rate_by_clocks(GB_gameboy_t *gb, double cycles_per_sample); /* Cycles are in 8MHz units */
void GB_set_highpass_filter_mode(GB_gameboy_t *gb, GB_highpass_mode_t mode);
void GB_set_interference_volume(GB_gameboy_t *gb, double volume);
void GB_apu_set_sample_callback(GB_gameboy_t *gb, GB_sample_callback_t callback);
int GB_start_audio_recording(GB_gameboy_t *gb, const char *path, GB_audio_format_t format);
int GB_stop_audio_recording(GB_gameboy_t *gb);
#ifdef GB_INTERNAL
internal bool GB_apu_is_DAC_enabled(GB_gameboy_t *gb, GB_channel_t index);
internal void GB_apu_write(GB_gameboy_t *gb, uint8_t reg, uint8_t value);
internal uint8_t GB_apu_read(GB_gameboy_t *gb, uint8_t reg);
internal void GB_apu_div_event(GB_gameboy_t *gb);
internal void GB_apu_div_secondary_event(GB_gameboy_t *gb);
internal void GB_apu_init(GB_gameboy_t *gb);
internal void GB_apu_run(GB_gameboy_t *gb, bool force);
#endif

#endif /* apu_h */
