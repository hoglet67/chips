#pragma once
/*#
    # atomx.h

    Acorn Atom emulator in a C header.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following macros with your own implementation

    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    You need to include the following headers before including atom.h:

    - chips/m6502.h
    - chips/mc6847.h
    - chips/i8255.h
    - chips/m6522.h
    - chips/beeper.h
    - chips/mem.h
    - chips/kbd.h
    - chips/clk.h

    ## The Acorn Atom

    FIXME!

    ## TODO

    - VIA emulation is currently only minimal
    - handle shift key (some games use this as jump button)
    - AtomMMC is very incomplete (only what's needed for joystick)

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATOM_FREQUENCY (1000000)
#define ATOM_MAX_AUDIO_SAMPLES (1024)       /* max number of audio samples in internal sample buffer */
#define ATOM_DEFAULT_AUDIO_SAMPLES (128)    /* default number of samples in internal sample buffer */
#define ATOM_MAX_TAPE_SIZE (1<<16)          /* max size of tape file in bytes */

/* joystick emulation types */
typedef enum {
    ATOM_JOYSTICKTYPE_NONE,
    ATOM_JOYSTICKTYPE_MMC
} atom_joystick_type_t;

/* sid emulation types */
typedef enum {
    ATOM_SIDTYPE_NONE,
    ATOM_SIDTYPE_M6581
} atom_sid_type_t;

/* joystick mask bits */
#define ATOM_JOYSTICK_RIGHT (1<<0)
#define ATOM_JOYSTICK_LEFT  (1<<1)
#define ATOM_JOYSTICK_DOWN  (1<<2)
#define ATOM_JOYSTICK_UP    (1<<3)
#define ATOM_JOYSTICK_BTN   (1<<4)

#define KEYCODE_SUPPRESS     0xFF

/* audio sample data callback */
typedef void (*atom_audio_callback_t)(const float* samples, int num_samples, void* user_data);

/* configuration parameters for atom_init() */
typedef struct {
    atom_joystick_type_t joystick_type;     /* what joystick type to emulate, default is ATOM_JOYSTICK_NONE */

    /* video output config */
    void* pixel_buffer;         /* pointer to a linear RGBA8 pixel buffer, at least 320*256*4 bytes */
    int pixel_buffer_size;      /* size of the pixel buffer in bytes */

    /* optional user-data for callbacks */
    void* user_data;

    /* audio output config (if you don't want audio, set audio_cb to zero) */
    atom_audio_callback_t audio_cb;   /* called when audio_num_samples are ready */
    int audio_num_samples;          /* default is ZX_AUDIO_NUM_SAMPLES */
    int audio_sample_rate;          /* playback sample rate, default is 44100 */
    float audio_volume;             /* audio volume: 0.0..1.0, default is 0.25 */

    /* ROM images */
    const void* rom_abasic;
    const void* rom_afloat;
    const void* rom_dosrom;
    int rom_abasic_size;
    int rom_afloat_size;
    int rom_dosrom_size;

    /* AtoMMC configuration */
    bool atommc_enabled;
    bool atommc_autoboot;
} atom_desc_t;

/* Acorn Atom emulation state */
typedef struct {
    uint64_t pins;
    m6502_t cpu;
    mc6847_t vdg;
    i8255_t ppi;
    m6522_t via;
    atommc_t atommc;
    m6581_t sid;
    beeper_t beeper;
    bool valid;
    int counter_2_4khz;
    int period_2_4khz;
    bool state_2_4khz;
    bool out_cass0;
    bool out_cass1;
    bool shift;
    bool ctrl;
    bool rept;
    atom_joystick_type_t joystick_type;
    atom_sid_type_t sid_type;
    uint8_t kbd_joymask;        /* joystick mask from keyboard-joystick-emulation */
    uint8_t joy_joymask;        /* joystick mask from calls to atom_joystick() */
    uint8_t mmc_cmd;
    uint8_t mmc_latch;
    mem_t mem;
    kbd_t kbd;
    void* user_data;
    atom_audio_callback_t audio_cb;
    int num_samples;
    int sample_pos;
    float sample_buffer[ATOM_MAX_AUDIO_SAMPLES];
    uint8_t ram[0xB000];
    uint8_t rom_abasic[0x2000];
    uint8_t rom_afloat[0x1000];
    uint8_t rom_dosrom[0x1000];
    /* break key handling */
    bool in_reset;
    /* tape loading */
    int tape_size;  /* tape_size is > 0 if a tape is inserted */
    int tape_pos;
    uint8_t tape_buf[ATOM_MAX_TAPE_SIZE];
    /* AtoMMC configuration */
    bool atommc_enabled;
    bool atommc_autoboot;
} atom_t;

/* initialize a new Atom instance */
void atom_init(atom_t* sys, const atom_desc_t* desc);
/* discard Atom instance */
void atom_discard(atom_t* sys);
/* get the standard framebuffer width and height in pixels */
int atom_std_display_width(void);
int atom_std_display_height(void);
/* get the maximum framebuffer size in number of bytes */
int atom_max_display_size(void);
/* get the current framebuffer width and height in pixels */
int atom_display_width(atom_t* sys);
int atom_display_height(atom_t* sys);
/* reset Atom instance */
void atom_reset(atom_t* sys);
/* execute a single tick */
void atom_tick(atom_t* sys);
/* run Atom instance for a number of microseconds */
void atom_exec(atom_t* sys, uint32_t micro_seconds);
/* send a key down event */
void atom_key_down(atom_t* sys, int key_code);
/* send a key up event */
void atom_key_up(atom_t* sys, int key_code);
/* enable/disable joystick emulation */
void atom_set_joystick_type(atom_t* sys, atom_joystick_type_t type);
/* get current joystick emulation type */
atom_joystick_type_t atom_joystick_type(atom_t* sys);
/* set joystick mask (combination of ATOM_JOYSTICK_*) */
void atom_joystick(atom_t* sys, uint8_t mask);
/* insert a tape for loading (must be an Atom TAP file), data will be copied */
bool atom_insert_tape(atom_t* sys, const uint8_t* ptr, int num_bytes);
/* remove tape */
void atom_remove_tape(atom_t* sys);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

#define _ATOM_ROM_DOSROM_SIZE (0x1000)

static uint64_t _atom_tick(atom_t* sys, uint64_t pins);
static uint64_t _atom_vdg_fetch(uint64_t pins, void* user_data);
static uint8_t _atom_ppi_in(int port_id, void* user_data);
static uint64_t _atom_ppi_out(int port_id, uint64_t pins, uint8_t data, void* user_data);
static uint8_t _atom_via_in(int port_id, void* user_data);
static void _atom_via_out(int port_id, uint8_t data, void* user_data);
static uint8_t _atom_atommc_in(int port_id, void* user_data);
static void _atom_atommc_out(int port_id, uint8_t data, void* user_data);
static void _atom_init_keymap(atom_t* sys);
static void _atom_init_memorymap(atom_t* sys);
static uint64_t _atom_osload(atom_t* sys, uint64_t pins);

#define _ATOM_DEFAULT(val,def) (((val) != 0) ? (val) : (def))
#define _ATOM_CLEAR(val) memset(&val, 0, sizeof(val))

void atom_init(atom_t* sys, const atom_desc_t* desc) {
    CHIPS_ASSERT(sys && desc);
    CHIPS_ASSERT(desc->pixel_buffer && (desc->pixel_buffer_size >= atom_max_display_size()));

    memset(sys, 0, sizeof(atom_t));
    sys->valid = true;
    sys->joystick_type = desc->joystick_type;
    sys->sid_type = ATOM_SIDTYPE_NONE;
    sys->user_data = desc->user_data;
    sys->audio_cb = desc->audio_cb;
    sys->num_samples = _ATOM_DEFAULT(desc->audio_num_samples, ATOM_DEFAULT_AUDIO_SAMPLES);
    sys->atommc_enabled = desc->atommc_enabled;
    sys->atommc_autoboot = desc->atommc_autoboot;
    CHIPS_ASSERT(sys->num_samples <= ATOM_MAX_AUDIO_SAMPLES);
    CHIPS_ASSERT(desc->rom_abasic && (desc->rom_abasic_size == sizeof(sys->rom_abasic)));
    memcpy(sys->rom_abasic, desc->rom_abasic, sizeof(sys->rom_abasic));
    CHIPS_ASSERT(desc->rom_afloat && (desc->rom_afloat_size == sizeof(sys->rom_afloat)));
    memcpy(&sys->rom_afloat, desc->rom_afloat, sizeof(sys->rom_afloat));
    CHIPS_ASSERT(desc->rom_dosrom && (desc->rom_dosrom_size == sizeof(sys->rom_dosrom)));
    memcpy(&sys->rom_dosrom, desc->rom_dosrom, sizeof(sys->rom_dosrom));

    /* initialize the hardware */
    sys->period_2_4khz = ATOM_FREQUENCY / 4800;

    m6502_desc_t cpu_desc;
    _ATOM_CLEAR(cpu_desc);
    sys->pins = m6502_init(&sys->cpu, &cpu_desc);

    mc6847_desc_t vdg_desc;
    _ATOM_CLEAR(vdg_desc);
    vdg_desc.tick_hz = ATOM_FREQUENCY;
    vdg_desc.rgba8_buffer = (uint32_t*) desc->pixel_buffer;
    vdg_desc.rgba8_buffer_size = desc->pixel_buffer_size;
    vdg_desc.fetch_cb = _atom_vdg_fetch;
    vdg_desc.user_data = sys;
    mc6847_init(&sys->vdg, &vdg_desc);

    i8255_desc_t ppi_desc;
    _ATOM_CLEAR(ppi_desc);
    ppi_desc.in_cb = _atom_ppi_in;
    ppi_desc.out_cb = _atom_ppi_out;
    ppi_desc.user_data = sys;
    i8255_init(&sys->ppi, &ppi_desc);

    m6522_desc_t via_desc;
    _ATOM_CLEAR(via_desc);
    via_desc.in_cb = _atom_via_in;
    via_desc.out_cb = _atom_via_out;
    via_desc.user_data = sys;
    m6522_init(&sys->via, &via_desc);

    if (sys->atommc_enabled) {
       atommc_desc_t atommc_desc;
       _ATOM_CLEAR(atommc_desc);
       atommc_desc.in_cb = _atom_atommc_in;
       atommc_desc.out_cb = _atom_atommc_out;
       atommc_desc.user_data = sys;
       atommc_desc.autoboot = sys->atommc_autoboot;
       atommc_init(&sys->atommc, &atommc_desc);
    }

    const int audio_hz = _ATOM_DEFAULT(desc->audio_sample_rate, 44100);
    const float audio_vol = _ATOM_DEFAULT(desc->audio_volume, 0.5f);
    beeper_init(&sys->beeper, ATOM_FREQUENCY, audio_hz, audio_vol);

    m6581_desc_t sid_desc;
    _ATOM_CLEAR(sid_desc);
    sid_desc.tick_hz = 1000000;
    sid_desc.sound_hz = audio_hz;
    sid_desc.magnitude = 1.0;
    m6581_init(&sys->sid, &sid_desc);

    /* setup memory map and keyboard matrix */
    _atom_init_memorymap(sys);
    _atom_init_keymap(sys);
}

void atom_discard(atom_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->valid = false;
}

int atom_std_display_width(void) {
    return MC6847_DISPLAY_WIDTH;
}

int atom_std_display_height(void) {
    return MC6847_DISPLAY_HEIGHT;
}

int atom_max_display_size(void) {
    return MC6847_DISPLAY_WIDTH * MC6847_DISPLAY_HEIGHT * 4;
}

int atom_display_width(atom_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return MC6847_DISPLAY_WIDTH;
}

int atom_display_height(atom_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return MC6847_DISPLAY_HEIGHT;
}

void atom_reset(atom_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->pins |= M6502_RES;
    i8255_reset(&sys->ppi);
    m6522_reset(&sys->via);
    if (sys->atommc_enabled) {
       atommc_reset(&sys->atommc);
    }
    mc6847_reset(&sys->vdg);
    beeper_reset(&sys->beeper);
    m6581_reset(&sys->sid);
    sys->state_2_4khz = false;
    sys->out_cass0 = false;
    sys->out_cass1 = false;
}

void atom_tick(atom_t* sys) {
    sys->pins = _atom_tick(sys, sys->pins);
}

void atom_exec(atom_t* sys, uint32_t micro_seconds) {
    CHIPS_ASSERT(sys && sys->valid);
    uint32_t num_ticks = clk_us_to_ticks(ATOM_FREQUENCY, micro_seconds);
    for (uint32_t ticks = 0; ticks < num_ticks; ticks++) {
        sys->pins = _atom_tick(sys, sys->pins);
    }
    kbd_update(&sys->kbd);
}

int handle_shift_ctrl_rept_break(atom_t* sys, int key_code, bool val) {

   // Handle special keys, like shift, control, repeat and break
   switch (key_code) {
   case SAPP_KEYCODE_F10:
   case SAPP_KEYCODE_F12:
      sys->in_reset = val;
      atom_reset(sys);
      break;
   case SAPP_KEYCODE_LEFT_SHIFT:
   case SAPP_KEYCODE_RIGHT_SHIFT:
      sys->shift = val;
      break;
   case SAPP_KEYCODE_LEFT_CONTROL:
   case SAPP_KEYCODE_RIGHT_CONTROL:
      sys->ctrl = val;
      break;
   case SAPP_KEYCODE_RIGHT_ALT:
   case SAPP_KEYCODE_LEFT_ALT:
   case SAPP_KEYCODE_KP_0:
      sys->rept = val;
      break;
   }

   // Remap key codes, as key matrix has maximum of 256 keys
   int remapped_key_code = key_code >= 256 ? key_code - 128 : key_code;

   // Handle AtoMMC Joystick
   int return_key_code;
   if (sys->joystick_type == ATOM_JOYSTICKTYPE_MMC) {
      return_key_code = KEYCODE_SUPPRESS;
      switch (key_code) {
      case SAPP_KEYCODE_SPACE:
         if (val) {
            sys->kbd_joymask |= ATOM_JOYSTICK_BTN;
         } else {
            sys->kbd_joymask &= ~ATOM_JOYSTICK_BTN;
         }
         break;
      case SAPP_KEYCODE_LEFT:
         if (val) {
            sys->kbd_joymask |= ATOM_JOYSTICK_LEFT;
         } else {
            sys->kbd_joymask &= ~ATOM_JOYSTICK_LEFT;
         }
         break;
      case SAPP_KEYCODE_RIGHT:
         if (val) {
            sys->kbd_joymask |= ATOM_JOYSTICK_RIGHT;
         } else {
            sys->kbd_joymask &= ~ATOM_JOYSTICK_RIGHT;
         }
         break;
      case SAPP_KEYCODE_DOWN:
         if (val) {
            sys->kbd_joymask |= ATOM_JOYSTICK_DOWN;
         } else {
            sys->kbd_joymask &= ~ATOM_JOYSTICK_DOWN;
         }
         break;
      case SAPP_KEYCODE_UP:
         if (val) {
            sys->kbd_joymask |= ATOM_JOYSTICK_UP;
         } else {
            sys->kbd_joymask &= ~ATOM_JOYSTICK_UP;
         }
         break;
      default:
         return_key_code = remapped_key_code;
      }
   } else {
      return_key_code = remapped_key_code;
   }
   return return_key_code;
}

void atom_key_down(atom_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    // Handle shift/ctrl/rept/break, remap higher key codes, handle joystick
    key_code = handle_shift_ctrl_rept_break(sys, key_code, true);
    // Pass on to keyboard matrix
    if (key_code != KEYCODE_SUPPRESS) {
       kbd_key_down(&sys->kbd, key_code);
    }
}

void atom_key_up(atom_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    // Handle shift/ctrl/rept/break, remap higher key codes, handle joystick
    key_code = handle_shift_ctrl_rept_break(sys, key_code, false);
    // Pass on to keyboard matrix
    if (key_code != KEYCODE_SUPPRESS) {
       kbd_key_up(&sys->kbd, key_code);
    }
}

void atom_set_joystick_type(atom_t* sys, atom_joystick_type_t type) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->joystick_type = type;
}

atom_joystick_type_t atom_joystick_type(atom_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return sys->joystick_type;
}

void atom_joystick(atom_t* sys, uint8_t mask) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->joy_joymask = mask;
}


/* CPU tick callback */
uint64_t _atom_tick(atom_t* sys, uint64_t pins) {
    bool sample = false;

    /* tick the CPU */
    if (!sys->in_reset) {
        pins = m6502_tick(&sys->cpu, pins);
    }

    /* tick the video chip */
    mc6847_tick(&sys->vdg);

    /* tick the 6522 VIA */
    m6522_tick(&sys->via);

    /* tick the AtoMMC */
    if (sys->atommc_enabled) {
       atommc_tick(&sys->atommc);
    }

    /* tick the 2.4khz counter */
    sys->counter_2_4khz++;
    if (sys->counter_2_4khz >= sys->period_2_4khz) {
        sys->state_2_4khz = !sys->state_2_4khz;
        sys->counter_2_4khz -= sys->period_2_4khz;
    }

    /* update beeper */
    if (beeper_tick(&sys->beeper)) {
       sample = true;
    }

    if (sys->sid_type == ATOM_SIDTYPE_M6581) {
       if (m6581_tick(&sys->sid)) {
          sample = true;
       }
    }

    if (sample) {
        /* new audio sample ready */
        float sample = sys->sid.sample;
        sample += sys->beeper.sample;
        sys->sample_buffer[sys->sample_pos++] = sample;
        if (sys->sample_pos == sys->num_samples) {
            if (sys->audio_cb) {
                sys->audio_cb(sys->sample_buffer, sys->num_samples, sys->user_data);
            }
            sys->sample_pos = 0;
        }
    }

    /* decode address for memory-mapped IO and memory read/write */
    const uint16_t addr = M6502_GET_ADDR(pins);
    if ((addr >= 0xB000) && (addr < 0xC000)) {
        /* memory-mapped IO area */
        if ((addr >= 0xB000) && (addr < 0xB400)) {
            /* i8255 PPI: http://www.acornatom.nl/sites/fpga/www.howell1964.freeserve.co.uk/acorn/atom/amb/amb_8255.htm */
            uint64_t ppi_pins = (pins & M6502_PIN_MASK) | I8255_CS;
            if (pins & M6502_RW) { ppi_pins |= I8255_RD; }  /* PPI read access */
            else { ppi_pins |= I8255_WR; }                  /* PPI write access */
            if (pins & M6502_A0) { ppi_pins |= I8255_A0; }  /* PPI has 4 addresses (port A,B,C or control word */
            if (pins & M6502_A1) { ppi_pins |= I8255_A1; }
            pins = i8255_iorq(&sys->ppi, ppi_pins) & M6502_PIN_MASK;
        }
        else if ((addr >= 0xB400) && (addr < 0xB800)) {

           if (sys->atommc_enabled) {

              /* FixMe: should do this with pins! */
              sys->atommc.port_data = ~(sys->kbd_joymask | sys->joy_joymask);

              uint64_t atommc_pins = (pins & M6502_PIN_MASK)|ATOMMC_CS;
              /* NOTE: ATOMMC_RW pin is identical with M6502_RW) */
              pins = atommc_iorq(&sys->atommc, atommc_pins) & M6502_PIN_MASK;

           } else {

            /* a quick'n'dirty hack for joystick input */
              if (pins & M6502_RW) {
                 /* read from MMC extension */
                 if (addr == 0xB400) {
                    /* reading from 0xB400 returns a status/error code, the important
                        ones are STATUS_OK=0x3F, and STATUS_BUSY=0x80, STATUS_COMPLETE
                        together with an error code is used to communicate errors
                    */
                    M6502_SET_DATA(pins, 0x3F);
                 }
                 else if ((addr == 0xB401) && (sys->mmc_cmd == 0xA2)) {
                    /* read MMC joystick */
                    M6502_SET_DATA(pins, ~(sys->kbd_joymask | sys->joy_joymask));
                 }
              }
              else {
                 /* write to MMC extension */
                 if (addr == 0xB400) {
                    sys->mmc_cmd = M6502_GET_DATA(pins);
                 }
              }
           }
        }
        else if ((addr >= 0xB800) && (addr < 0xBC00)) {
            /* 6522 VIA: http://www.acornatom.nl/sites/fpga/www.howell1964.freeserve.co.uk/acorn/atom/amb/amb_6522.htm */
            uint64_t via_pins = (pins & M6502_PIN_MASK)|M6522_CS1;
            /* NOTE: M6522_RW pin is identical with M6502_RW) */
            pins = m6522_iorq(&sys->via, via_pins) & M6502_PIN_MASK;
        }
        else if ((addr >= 0xBDC0) && (addr < 0xBDE0)) {
            /* SID (BDC..BDDF) */
            uint64_t sid_pins = (pins & M6502_PIN_MASK)|M6581_CS;
            pins = m6581_iorq(&sys->sid, sid_pins) & M6502_PIN_MASK;
        }

        else {
            /* remaining IO space is for expansion devices */
            if (pins & M6502_RW) {
                M6502_SET_DATA(pins, 0x00);
            }
        }
    }
    else {
        /* regular memory access */
        if (pins & M6502_RW) {
            /* memory read */
            M6502_SET_DATA(pins, mem_rd(&sys->mem, addr));
        }
        else {
            /* memory access */
            mem_wr(&sys->mem, addr, M6502_GET_DATA(pins));
        }
    }

    /* check if the trapped OSLoad function was hit to implement tape file loading
        http://ladybug.xs4all.nl/arlet/fpga/6502/kernel.dis
    */
    if (sys->tape_size > 0) {
        const uint64_t trap_mask = M6502_SYNC|0xFFFF;
        const uint64_t trap_val  = M6502_SYNC|0xF96E;
        if ((pins & trap_mask) == trap_val) {
            pins = _atom_osload(sys, pins);
        }
    }
    return pins;
}

uint64_t _atom_vdg_fetch(uint64_t pins, void* user_data) {
    atom_t* sys = (atom_t*) user_data;
    const uint16_t addr = MC6847_GET_ADDR(pins);
    uint8_t data = sys->ram[(addr + 0x8000) & 0xFFFF];
    MC6847_SET_DATA(pins, data);

    /*  the upper 2 databus bits are directly wired to MC6847 pins:
        bit 7 -> INV pin (in text mode, invert pixel pattern)
        bit 6 -> A/S and INT/EXT pin, A/S actives semigraphics mode
                 and INT/EXT selects the 2x3 semigraphics pattern
                 (so 4x4 semigraphics isn't possible)
    */
    if (data & (1<<7)) { pins |= MC6847_INV; }
    else               { pins &= ~MC6847_INV; }
    if (data & (1<<6)) { pins |= (MC6847_AS|MC6847_INTEXT); }
    else               { pins &= ~(MC6847_AS|MC6847_INTEXT); }
    return pins;
}

uint64_t _atom_ppi_out(int port_id, uint64_t pins, uint8_t data, void* user_data) {
    atom_t* sys = (atom_t*) user_data;
    /*
        FROM Atom Theory and Praxis (and MAME)
        The  8255  Programmable  Peripheral  Interface  Adapter  contains  three
        8-bit ports, and all but one of these lines is used by the ATOM.
        Port A - #B000
               Output bits:      Function:
                    O -- 3     Keyboard column
                    4 -- 7     Graphics mode (4: A/G, 5..7: GM0..2)
        Port B - #B001
               Input bits:       Function:
                    O -- 5     Keyboard row
                      6        CTRL key (low when pressed)
                      7        SHIFT keys {low when pressed)
        Port C - #B002
               Output bits:      Function:
                    O          Tape output
                    1          Enable 2.4 kHz to cassette output
                    2          Loudspeaker
                    3          Not used (??? see below)
               Input bits:       Function:
                    4          2.4 kHz input
                    5          Cassette input
                    6          REPT key (low when pressed)
                    7          60 Hz sync signal (low during flyback)
        The port C output lines, bits O to 3, may be used for user
        applications when the cassette interface is not being used.
    */
    if (I8255_PORT_A == port_id) {
        /* PPI port A
            0..3:   keyboard matrix column to scan next
            4:      MC6847 A/G
            5:      MC6847 GM0
            6:      MC6847 GM1
            7:      MC6847 GM2
        */
        kbd_set_active_columns(&sys->kbd, 1<<(data & 0x0F));
        uint64_t vdg_pins = 0;
        uint64_t vdg_mask = MC6847_AG|MC6847_GM0|MC6847_GM1|MC6847_GM2;
        if (data & (1<<4)) { vdg_pins |= MC6847_AG; }
        if (data & (1<<5)) { vdg_pins |= MC6847_GM0; }
        if (data & (1<<6)) { vdg_pins |= MC6847_GM1; }
        if (data & (1<<7)) { vdg_pins |= MC6847_GM2; }
        mc6847_ctrl(&sys->vdg, vdg_pins, vdg_mask);
    }
    else if (I8255_PORT_C == port_id) {
        /* PPI port C output:
            0:  output: cass 0
            1:  output: cass 1
            2:  output: speaker
            3:  output: MC6847 CSS

            NOTE: only the MC6847 CSS pin is emulated here
        */
        sys->out_cass0 = 0 == (data & (1<<0));
        sys->out_cass1 = 0 == (data & (1<<1));
        beeper_set(&sys->beeper, 0 == (data & (1<<2)));
        uint64_t vdg_pins = 0;
        uint64_t vdg_mask = MC6847_CSS;
        if (data & (1<<3)) {
            vdg_pins |= MC6847_CSS;
        }
        mc6847_ctrl(&sys->vdg, vdg_pins, vdg_mask);
    }
    return pins;
}

uint8_t _atom_ppi_in(int port_id, void* user_data) {
    atom_t* sys = (atom_t*) user_data;
    uint8_t data = 0;
    if (I8255_PORT_B == port_id) {
        /* keyboard row state */
       data = ~((sys->shift << 7) | (sys->ctrl << 6) | (kbd_scan_lines(&sys->kbd) & 0x3f));
    }
    else if (I8255_PORT_C == port_id) {
        /*  PPI port C input:
            4:  input: 2400 Hz
            5:  input: cassette
            6:  input: keyboard repeat
            7:  input: MC6847 FSYNC

            NOTE: only the 2400 Hz oscillator and FSYNC pins is emulated here
        */
        if (sys->state_2_4khz) {
            data |= (1<<4);
        }
        /* FIXME: always send REPEAT key as 'not pressed' */
        data |= (sys->rept^1)<<6;
        /* vblank pin (cleared during vblank) */
        if (0 == (sys->vdg.pins & MC6847_FS)) {
            data |= (1<<7);
        }
    }
    return data;
}

static void _atom_via_out(int port_id, uint8_t data, void* user_data) {
    /* FIXME */
}

static uint8_t _atom_via_in(int port_id, void* user_data) {
    /* FIXME */
    return 0x00;
}

static void _atom_atommc_out(int port_id, uint8_t data, void* user_data) {
    /* FIXME */
}

static uint8_t _atom_atommc_in(int port_id, void* user_data) {
    /* FIXME */
    return 0x00;
}


static void _atom_init_keymap(atom_t* sys) {
    /*  setup the keyboard matrix
        the Atom has a 10x8 keyboard matrix, where the
        entire line 6 is for the Ctrl key, and the entire
        line 7 is the Shift key
    */
    kbd_init(&sys->kbd, 1);

    // Keycodes in the 256-383 range are mapped down to 128-255
    // as the keyboard matrix only support 256 key codes
    //
    //                                                                // Atom Key
    kbd_register_key(&sys->kbd,    SAPP_KEYCODE_ESCAPE-128, 0, 5, 0); // Escape
    kbd_register_key(&sys->kbd,                        'Z', 1, 5, 0); // Z
    kbd_register_key(&sys->kbd,                        'Y', 2, 5, 0); // Y
    kbd_register_key(&sys->kbd,                        'X', 3, 5, 0); // X
    kbd_register_key(&sys->kbd,                        'W', 4, 5, 0); // W
    kbd_register_key(&sys->kbd,                        'V', 5, 5, 0); // V
    kbd_register_key(&sys->kbd,                        'U', 6, 5, 0); // U
    kbd_register_key(&sys->kbd,                        'T', 7, 5, 0); // T
    kbd_register_key(&sys->kbd,                        'S', 8, 5, 0); // S
    kbd_register_key(&sys->kbd,                        'R', 9, 5, 0); // R
    kbd_register_key(&sys->kbd,                        'Q', 0, 4, 0); // Q
    kbd_register_key(&sys->kbd,                        'P', 1, 4, 0); // P
    kbd_register_key(&sys->kbd,                        'O', 2, 4, 0); // O
    kbd_register_key(&sys->kbd,                        'N', 3, 4, 0); // N
    kbd_register_key(&sys->kbd,                        'M', 4, 4, 0); // M
    kbd_register_key(&sys->kbd,                        'L', 5, 4, 0); // L
    kbd_register_key(&sys->kbd,                        'K', 6, 4, 0); // K
    kbd_register_key(&sys->kbd,                        'J', 7, 4, 0); // J
    kbd_register_key(&sys->kbd,                        'I', 8, 4, 0); // I
    kbd_register_key(&sys->kbd,                        'H', 9, 4, 0); // H
    kbd_register_key(&sys->kbd,                        'G', 0, 3, 0); // G
    kbd_register_key(&sys->kbd,                        'F', 1, 3, 0); // F
    kbd_register_key(&sys->kbd,                        'E', 2, 3, 0); // E
    kbd_register_key(&sys->kbd,                        'D', 3, 3, 0); // D
    kbd_register_key(&sys->kbd,                        'C', 4, 3, 0); // C
    kbd_register_key(&sys->kbd,                        'B', 5, 3, 0); // B
    kbd_register_key(&sys->kbd,                        'A', 6, 3, 0); // A
    kbd_register_key(&sys->kbd,                       '\'', 7, 3, 0); // @
    kbd_register_key(&sys->kbd,                        '/', 8, 3, 0); // Forward Slash
    kbd_register_key(&sys->kbd,                        '.', 9, 3, 0); // .
    kbd_register_key(&sys->kbd,                        '-', 0, 2, 0); // -
    kbd_register_key(&sys->kbd,                        ',', 1, 2, 0); // ,
    kbd_register_key(&sys->kbd,                        ';', 2, 2, 0); // ;
    kbd_register_key(&sys->kbd,                        '=', 3, 2, 0); // :
    kbd_register_key(&sys->kbd,                        '9', 4, 2, 0); // 9
    kbd_register_key(&sys->kbd,                        '8', 5, 2, 0); // 8
    kbd_register_key(&sys->kbd,                        '7', 6, 2, 0); // 7
    kbd_register_key(&sys->kbd,                        '6', 7, 2, 0); // 6
    kbd_register_key(&sys->kbd,                        '5', 8, 2, 0); // 5
    kbd_register_key(&sys->kbd,                        '4', 9, 2, 0); // 4
    kbd_register_key(&sys->kbd,                        '3', 0, 1, 0); // 3
    kbd_register_key(&sys->kbd,                        '2', 1, 1, 0); // 2
    kbd_register_key(&sys->kbd,                        '1', 2, 1, 0); // 1
    kbd_register_key(&sys->kbd,                        '0', 3, 1, 0); // 0
    kbd_register_key(&sys->kbd, SAPP_KEYCODE_BACKSPACE-128, 4, 1, 0); // Del        (mapped to Backspace)
    kbd_register_key(&sys->kbd,       SAPP_KEYCODE_END-128, 5, 1, 0); // Copy       (mapped to End)
    kbd_register_key(&sys->kbd,     SAPP_KEYCODE_ENTER-128, 6, 1, 0); // Return     (mapped to Return
    kbd_register_key(&sys->kbd,        SAPP_KEYCODE_UP-128, 2, 0, 0); // Up/Down    (mapped to Up Arrow)
    kbd_register_key(&sys->kbd,      SAPP_KEYCODE_MENU-128, 2, 0, 0); // Up/Down    (mapped to Menu)
    kbd_register_key(&sys->kbd,     SAPP_KEYCODE_RIGHT-128, 3, 0, 0); // Right/Left (mapped to Right Arrow)
    kbd_register_key(&sys->kbd, SAPP_KEYCODE_CAPS_LOCK-128, 4, 0, 0); // Caps       (mapped to Caps Lock)
    kbd_register_key(&sys->kbd,       SAPP_KEYCODE_TAB-128, 5, 0, 0); // ^          (mapped to Tab)
    kbd_register_key(&sys->kbd,                        ']', 6, 0, 0); // ]
    kbd_register_key(&sys->kbd,                       '\\', 7, 0, 0); // Back Slash
    kbd_register_key(&sys->kbd,                        '[', 8, 0, 0); // ]
    kbd_register_key(&sys->kbd,                        ' ', 9, 0, 0); // Space
}

static uint32_t _atom_xorshift32(uint32_t x) {
    x ^= x<<13;
    x ^= x>>17;
    x ^= x<<5;
    return x;
}

static void _atom_init_memorymap(atom_t* sys) {
    mem_init(&sys->mem);

    /* fill memory with random junk */
    uint32_t r = 0x6D98302B;
    for (int i = 0; i < (int)sizeof(sys->ram);) {
        r = _atom_xorshift32(r);
        sys->ram[i++] = r;
        sys->ram[i++] = (r>>8);
        sys->ram[i++] = (r>>16);
        sys->ram[i++] = (r>>24);
    }
    /* 32 KB RAM (with RAM extension) + 8 KB vidmem + 4K Utility ROM*/
    mem_map_ram(&sys->mem, 0, 0x0000, 0xB000, sys->ram);
    /* hole in 0xA000 to 0xAFFF (for utility ROMs) */
    /* 0xB000 to 0xBFFF: IO area, not mapped */
    /* 16 KB ROMs from 0xC000 */
    mem_map_rom(&sys->mem, 0, 0xC000, 0x1000, sys->rom_abasic);
    mem_map_rom(&sys->mem, 0, 0xD000, 0x1000, sys->rom_afloat);
    mem_map_rom(&sys->mem, 0, 0xE000, 0x1000, sys->rom_dosrom);
    mem_map_rom(&sys->mem, 0, 0xF000, 0x1000, sys->rom_abasic + 0x1000);
}

/*=== FILE LOADING ===========================================================*/
/* Atom TAP / ATM header (https://github.com/hoglet67/Atomulator/blob/master/docs/atommmc2.txt ) */
typedef struct {
    uint8_t name[16];
    uint16_t load_addr;
    uint16_t exec_addr;
    uint16_t length;
} _atom_tap_header;

bool atom_insert_tape(atom_t* sys, const uint8_t* ptr, int num_bytes) {
    CHIPS_ASSERT(sys && sys->valid);
    CHIPS_ASSERT(ptr);
    atom_remove_tape(sys);
    /* check for valid size */
    if ((num_bytes < (int)sizeof(_atom_tap_header)) || (num_bytes > ATOM_MAX_TAPE_SIZE)) {
        return false;
    }
    memcpy(sys->tape_buf, ptr, num_bytes);
    sys->tape_pos = 0;
    sys->tape_size = num_bytes;
    return true;
}

void atom_remove_tape(atom_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->tape_pos = 0;
    sys->tape_size = 0;
}

/*
    trapped OSLOAD function, load ATM block in a TAP file:
      https://github.com/hoglet67/Atomulator/blob/master/docs/atommmc2.tx
    from: http://ladybug.xs4all.nl/arlet/fpga/6502/kernel.di
      OSLOAD Load File subroutine
       --------------------------
     - Entry: 0,X = LSB File name string address
              1,X = MSB File name string address
              2,X = LSB Data dump start address
              3,X = MSB Data dump start address
              4,X : If bit 7 is clear, then the file's own start address is
                    to be used
              #DD = FLOAD flag - bit 7 is set if in FLOAD mod
     - Uses:  #C9 = LSB File name string address
              #CA = MSB File name string address
              #CB = LSB Data dump start address
              #CC = MSB Data dump start address
              #CD = load flag - if bit 7 is set, then the load address at
                    (#CB) is to be used instead of the file's load address
              #D0 = MSB Current block number
              #D1 = LSB Current block numbe
     - Header format: <*>                      )
                      <*>                      )
                      <*>                      )
                      <*>                      ) Header preamble
                      <Filename>               ) Name is 1 to 13 bytes long
                      <Status Flag>            ) Bit 7 clear if last block
                                               ) Bit 6 clear to skip block
                                               ) Bit 5 clear if first block
                      <LSB block number>
                      <MSB block number>       ) Always zero
                      <Bytes in block>
                      <MSB run address>
                      <LSB run address>
                      <MSB block load address>
                      <LSB block load address
     - Data format:   <....data....>           ) 1 to #FF bytes
                      <Checksum>               ) LSB sum of all data byte
*/
uint64_t _atom_osload(atom_t* sys, uint64_t pins) {
    bool success = false;

    /* tape inserted? */
    uint16_t exec_addr = 0;
    if ((sys->tape_size > 0) && (sys->tape_pos < sys->tape_size)) {
        /* read next tape chunk */
        if ((int)(sys->tape_pos + sizeof(_atom_tap_header)) < sys->tape_size) {
            const _atom_tap_header* hdr = (const _atom_tap_header*) &sys->tape_buf[sys->tape_pos];
            sys->tape_pos += sizeof(_atom_tap_header);
            exec_addr = hdr->exec_addr;
            uint16_t addr = hdr->load_addr;
            /* override file load address? */
            if (mem_rd(&sys->mem, 0xCD) & 0x80) {
                addr = mem_rd16(&sys->mem, 0xCB);
            }
            if ((sys->tape_pos + hdr->length) <= sys->tape_size) {
                for (int i = 0; i < hdr->length; i++) {
                    mem_wr(&sys->mem, addr++, sys->tape_buf[sys->tape_pos++]);
                }
                success = true;
            }
        }
    }
    /* if tape at end, remove tape */
    if (sys->tape_pos >= sys->tape_size) {
        atom_remove_tape(sys);
    }
    /* success/fail: set or clear bit 6 and clear bit 7 of 0xDD */
    uint8_t dd = mem_rd(&sys->mem, 0xDD);
    if (success) {
        dd |= (1<<6);
    }
    else {
        dd &= ~(1<<6);
    }
    dd &= ~(1<<7);
    mem_wr(&sys->mem, 0xDD, dd);

    if (success) {
        /* on success, continue with start of loaded code */
        sys->cpu.S += 2;
        M6502_SET_ADDR(pins, exec_addr);
        M6502_SET_DATA(pins, mem_rd(&sys->mem, exec_addr));
        m6502_set_pc(&sys->cpu, exec_addr);
    }
    else {
        /* otherwise just continue with an RTS */
        M6502_SET_ADDR(pins, 0xF9A1);
        M6502_SET_DATA(pins, mem_rd(&sys->mem, 0xF9A1));
        m6502_set_pc(&sys->cpu, 0xF9A1);
    }
    return pins;
}

#endif /* CHIPS_IMPL */
