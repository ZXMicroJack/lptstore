#include <stdio.h>
#include <stdint.h>
#include <pico/time.h>
#include "wtsynth.h"

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "hardware/flash.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 44100
#endif
#ifndef SAMPLE_LEN
#define SAMPLE_LEN 1024
#endif

#ifdef MAINDEBUG
#define DEBUG
#endif

#ifdef SYSEXDEBUG
#define SDEBUG
#endif


#ifdef DEBUG
#define debug(a) printf a
#else
#define debug(a)
#endif

#ifdef SDEBUG
#define sdebug(a) printf a
#else
#define sdebug(a)
#endif

bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));
#define SAMPLES_PER_BUFFER SAMPLE_LEN*2

int nrbuffs = 0;

struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .sample_freq = SAMPLE_RATE,
            .channel_count = 2,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 2
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;
    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);
    return producer_pool;
}

extern int tonesinuse;
extern int this_chan;

volatile uint8_t muteAudio = 0;
volatile uint8_t beep = 0;
volatile uint8_t beepCount = 0;
volatile uint32_t beepPattern = 0;
volatile uint8_t beepPatternCount = 0;
volatile uint32_t beepPatternAcc = 0;
volatile uint8_t beepPatternCountAcc = 0;
volatile uint8_t patternCount = 0;


void stopBeep() {
  beepPatternAcc = beepPattern = 0;
  beepPatternCountAcc = beepPatternCount = 0;
  beep = 0;
  patternCount = 0;
}


#if 0
volatile uint8_t audio_enabled = 1;
void audio_pause(int enable) {
  if (audio_enabled && !enable) {
    // switch off
  } else if (!audio_enabled && enable) {
    // switch on
  }
}
#endif

void audio_core() {
	short data[4096];
  struct audio_buffer_pool *ap = init_audio();
  
  for(;;) {
    // read and process audio data
    struct audio_buffer *buffer = take_audio_buffer(ap, true);
    int16_t *samples = (int16_t *) buffer->buffer->bytes;
    if (!muteAudio) {
      wtsynth_GetAudioPacket(samples);
    } else {
      for (int i=0; i<buffer->max_sample_count; i++) {
        samples[i] = beep == 1 ? ((i & 63) > 31 ? -10000 : 10000) :
          beep == 2 ? ((i & 127) > 63 ? -10000 : 10000) : 0;
#ifdef ZXUNO
        samples[i] ^= 0x8000;
#endif
      }
      gpio_put(PICO_DEFAULT_LED_PIN, beep ? 1 : 0);

      if (beepCount) {
        beepCount --;
        if (!beepCount) {
          beep = beepPatternAcc & 3;
          beepPatternAcc >>= 2;
          beepCount = beep ? 5 : 40;
          
          if (beepPatternCountAcc) {
            beepPatternCountAcc--;
            if (!beepPatternCountAcc) {
              if (patternCount) {
                patternCount --;
                if (!patternCount) {
                  stopBeep();
                } else {
                  beepPatternCountAcc = beepPatternCount;
                  beepPatternAcc = beepPattern;
                }
              }
            }
          }
        }
      }
    }
    buffer->sample_count = buffer->max_sample_count>>1;
    give_audio_buffer(ap, buffer);
    nrbuffs++;
  }
}

uint8_t *LUTS_POS = (uint8_t *)0x10040000;
uint8_t *SOUNDFONT_POS = (uint8_t *)0x100a0000;
uint8_t *SOUNDFONT2_POS = (uint8_t *)0x10200000;


// uint8_t *LUTS_POS = (uint8_t *)0x10040004;
// uint8_t *SOUNDFONT_POS = (uint8_t *)0x100a0004;
// uint8_t *SOUNDFONT2_POS = (uint8_t *)0x10200004;

#if PICO_NO_FLASH
static void enable_xip(void) {
  rom_connect_internal_flash_fn connect_internal_flash = (rom_connect_internal_flash_fn)rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH);
    rom_flash_exit_xip_fn flash_exit_xip = (rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
    rom_flash_flush_cache_fn flash_flush_cache = (rom_flash_flush_cache_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);
    rom_flash_enter_cmd_xip_fn flash_enter_cmd_xip = (rom_flash_enter_cmd_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_ENTER_CMD_XIP);

    connect_internal_flash();    // Configure pins
    flash_exit_xip();            // Init SSI, prepare flash for command mode
    flash_flush_cache();         // Flush & enable XIP cache
    flash_enter_cmd_xip();       // Configure SSI with read cmd
}
#endif

static void set_xip(int on) {
  rom_flash_exit_xip_fn flash_exit_xip = (rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
  rom_flash_flush_cache_fn flash_flush_cache = (rom_flash_flush_cache_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);
  rom_flash_enter_cmd_xip_fn flash_enter_cmd_xip = (rom_flash_enter_cmd_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_ENTER_CMD_XIP);

  if (!on) {
    flash_exit_xip();            // Init SSI, prepare flash for command mode
  } else {
    flash_flush_cache();         // Flush & enable XIP cache
    flash_enter_cmd_xip();       // Configure SSI with read cmd
  }
}



void blinkDeath(int ret) {
  beepPatternAcc = beepPattern = 0x11111111 & ~(0xf << (4*ret));
  beepPatternCountAcc = beepPatternCount = ret*2 + 1;
  beepCount = 1;
  patternCount = 5;
}

void beepSuccess() {
  beepPatternAcc = beepPattern = 0x18618618;
  beepPatternCountAcc = beepPatternCount = 15;
  beepCount = 1;
  patternCount = 3;
}

void beepFail() {
  beepPatternAcc = beepPattern = 0x28A28A28;
  beepPatternCountAcc = beepPatternCount = 15;
  beepCount = 1;
  patternCount = 3;
}

/* handle sysex control */
#define MIDI_SYSEX_START 		0xF0
#define MIDI_SYSEX_END	 		0xF7

uint8_t sysex_chksum;
uint8_t sysex_insysex = 0;
#define MAX_SYSEX   (254)
uint8_t sysex_buffer[MAX_SYSEX];

#define CMD_ECHO            0x01
#define CMD_STOPSYNTH       0x02
#define CMD_STARTSYNTH      0x03
#define CMD_BEEP            0x04
#define CMD_BUFFERALLOC     0x05
#define CMD_BUFFERFREE      0x06
#define CMD_WRITEDATA       0x07
#define CMD_CHECKCRC16      0x08
#define CMD_VERIFYPROGRAM   0x09
#define CMD_RESET           0x0A
#define CMD_SETBAUDRATE     0x0B

uint8_t *codeBuff = NULL;
uint32_t codeLen = 0;

void safeFreeCodeBuff() {
  if (codeBuff) {
    free(codeBuff);
    codeBuff = NULL;
    codeLen = 0;
  }
}

uint16_t crc16(const uint8_t* data_p, uint32_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }
    return crc;
}

void sysex_Process() {
  switch(sysex_buffer[0]) {
    case CMD_ECHO:
      printf("SysexEcho: ");
      for (int i=1; i<(sysex_insysex-3); i++) {
        printf("%02X ", sysex_buffer[i]);
      }
      printf("\n");
      break;

    case CMD_STOPSYNTH:
      muteAudio = 1;
      wtsynth_Kill();
      printf("sysex: Stopped synth\n");
      stopBeep();
      break;

    case CMD_STARTSYNTH:
      safeFreeCodeBuff();
      stopBeep();

      if (wtsynth_Init() >= 0) {
        muteAudio = 0;
        printf("sysex: Restarted synth\n");
      } else {
        printf("sysex: Failed to restart synth\n");
      }
      break;
    
    case CMD_BEEP:
      beepPatternCountAcc = beepPatternCount = sysex_buffer[1];
      beepPatternAcc = beepPattern = (sysex_buffer[2] << 26) |
        (sysex_buffer[3] << 20) |
        (sysex_buffer[4] << 14) |
        (sysex_buffer[5] << 8) |
        (sysex_buffer[6] << 2) |
        sysex_buffer[7];
      beepCount = sysex_buffer[1] ? 1 : 0; // kick off the beeping
      patternCount = sysex_buffer[8];
      sdebug(("sysex: beep - %08X %02X %02X\n", beepPattern, sysex_buffer[1], sysex_buffer[8]));
      break;
      
    case CMD_BUFFERALLOC:
      safeFreeCodeBuff();
      codeLen = sysex_buffer[1] * 4096;
      codeBuff = (uint8_t *)malloc(codeLen);
      break;

    case CMD_BUFFERFREE:
      safeFreeCodeBuff();
      break;
    
    case CMD_WRITEDATA: {
      uint16_t offset = (sysex_buffer[1] * 4096) + (sysex_buffer[2] * 32);
      for (int i=3; i<(sysex_insysex-3); i+=10) {
        sdebug(("offset:%04X ", offset));
        uint8_t msb = (sysex_buffer[i+8] << 4) | sysex_buffer[i+9];
        for (int j=0; j<8; j++) {
          codeBuff[offset] = sysex_buffer[i+j] | (msb & 0x80);
          msb <<= 1;
          sdebug(("%02X ", codeBuff[offset]));
          offset ++;
        }
        sdebug(("\n"));
      }
      break;
    }
    
    case CMD_CHECKCRC16: {
      uint16_t crc =  (sysex_buffer[1] << 8) | 
                      sysex_buffer[2] | 
                      ((sysex_buffer[5] & 0x08) << 12) |
                      ((sysex_buffer[5] & 0x04) << 5);
      uint16_t len =  (sysex_buffer[3] << 8) | 
                      sysex_buffer[4] | 
                      ((sysex_buffer[5] & 0x02) << 14) |
                      ((sysex_buffer[5] & 0x01) << 7);
      uint16_t calc = crc16(codeBuff, len == 0 ? 65536 : len);
      sdebug(("crc: len:%04X calc:%04X crc:%04X\n", len, calc, crc));
      if (calc == crc) {
        beepSuccess();
      } else {
        beepFail();
      }
      break;
    }

    case CMD_VERIFYPROGRAM: {
      uint8_t blk[8];
      uint8_t msb;
      
      msb = (sysex_buffer[9] << 4) | sysex_buffer[10];
      memcpy(blk, &sysex_buffer[1], 8);
      for (int i=0; i<8; i++) {
        blk[i] |= msb & 0x80;
        msb <<= 1;
      }
      
      uint16_t crc = (blk[0] << 8) | blk[1];
      uint16_t len = (blk[2] << 8) | blk[3];
      uint32_t addr = (blk[4] << 24) | (blk[5] << 16) | (blk[6] << 8) | blk[7];
      uint16_t calc = crc16(codeBuff, len == 0 ? 65536 : len);

      if (calc == crc && addr >= XIP_BASE && addr < (XIP_BASE + 0x01000000)) {
        uint32_t rlen = len == 0 ? 65536 : len;
        uint32_t elen = (rlen + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
        uint32_t plen = (rlen + FLASH_PAGE_SIZE - 1) & (~(FLASH_PAGE_SIZE - 1));
        
        sdebug(("rlen:%08X elen:%08X plen:%08X\n", rlen, elen, plen));

        set_xip(0);

        printf("Erasing ...\n");
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(addr & 0xffffff, elen);
        restore_interrupts(ints);
        printf("Programming...\n");
        ints = save_and_disable_interrupts();
        flash_range_program(addr & 0xffffff, codeBuff, plen);
        restore_interrupts(ints);

        printf("Checking...\n");
        set_xip(1);
        
        uint16_t crcFlash;

        // not sure why this is needed - but first time crc is calculated, it's wrong.
        int retries = 5;
        do {
          crcFlash = crc16((uint8_t *)addr, len == 0 ? 65536 : len);
          retries --;
          if (!retries) break;
        } while (crcFlash != crc);
        
#ifdef SDEBUG
        uint16_t crcCB = crc16((uint8_t *)codeBuff, len == 0 ? 65536 : len);
#endif
        sdebug(("verify flash: crc:%04X len:%04X addr:%08X flash:%04X cb:%04X\n", 
               crc, len, addr, crcFlash, crcCB));
        if (crcFlash == crc) {
          if (sysex_buffer[11] == 0x55) { // meaning reboot
            watchdog_enable(1, 1);
            for(;;);
          } else {
            beepSuccess();
          }
        } else {
          beepFail();
#ifdef SDEBUG
          printf("Listing all differences in binary: from len: %d\n", len);
          for (int i=0; i<len; i++) {
            uint8_t flashbyte = *(uint8_t *)(addr + i);
            if (codeBuff[i] != flashbyte) {
              printf("offset:%04X : f:%02X b:%02X\n", i, codeBuff[i], flashbyte);
            }
          }
          printf("Differences ends\n", len);
          crcFlash = crc16((uint8_t *)addr, len == 0 ? 65536 : len);
          printf("verify flash: crc:%04X len:%04X addr:%08X flash:%04X cb:%04X\n", 
                crc, len, addr, crcFlash, crcCB);
#endif
        }
        
      }
      
      sdebug(("verify: crc:%04X len:%04X addr:%08X calc:%04X\n", crc, len, addr, calc));
      break;
    }
    
    case CMD_SETBAUDRATE: {
      uint32_t baudRate = (sysex_buffer[1] << 21) | (sysex_buffer[2] << 14) 
        | (sysex_buffer[3] << 7) | sysex_buffer[4];
      uart_set_baudrate(uart1, baudRate);
      printf("sysex: set baudrate to %u\n", baudRate);
      break;
    }
    
    case CMD_RESET: {
      watchdog_enable(1, 1);
      for(;;);
      break;
    }
      
    default:
      printf("Unknown sysex cmd: %02X\n", sysex_buffer[0]);
  }
}

void wtsynth_Sysex(uint8_t data) {
//   printf("sysex: %02X\n", data);
  if (sysex_insysex && data == MIDI_SYSEX_END) {
    sdebug(("sysex: completed len = %d\n", sysex_insysex - 2));
    if (sysex_chksum == 0x00) sysex_Process();
#ifdef SDEBUG
    else printf("Sysex message failed checksum\n");
#endif
    sysex_insysex = 0;
  } else if (!sysex_insysex && data == MIDI_SYSEX_START) {
    sysex_insysex = 1;
    sysex_chksum = 0x47;
  } else if (sysex_insysex == 1) {
    // fairly sure a fairlight will never be connected to a zxuno
    if (data == 0x14) {
      sysex_insysex ++;
    } else {
      sdebug(("Sysex not for us!\n"));
      sysex_insysex = 0xff;
    }
  } else if (sysex_insysex > 0 && sysex_insysex < (MAX_SYSEX+2)) {
    sysex_buffer[sysex_insysex-2] = data;
    sysex_chksum ^= data;
    sysex_insysex ++;
  }
}

int main()
{
  stdio_init_all();
  sleep_ms(1000); // usb settle delay
  
#if PICO_NO_FLASH
  enable_xip();
#endif
  
  uart_init (uart1, 31250);
  uart_set_fifo_enabled(uart1, true);
  gpio_set_function(4, GPIO_FUNC_UART);
  gpio_set_function(5, GPIO_FUNC_UART);

  // set up error led
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  
  uint64_t started;
  
  muteAudio = 1;
  multicore_reset_core1();
  multicore_launch_core1(audio_core);

  int ret = wtsynth_Init();
  if (ret < 0) {
    ret = -ret;
    ret = 6;
    blinkDeath(ret);
  } else {
    muteAudio = 0;
  }

  int readable;
  unsigned char uartbuff[16];
  int thisread;
  
  for(;;) {
    int c = getchar_timeout_us(0);
    if (c == 'q') break;
    if (c == 'r') wtsynth_AllNotesOff();
    if (c == 'a') printf("nrbuffs %d\n", nrbuffs);

    // read and process midi data
    int readable = uart_is_readable(uart1);
    while (readable) {
      thisread = readable > sizeof uartbuff ? sizeof uartbuff : readable;
      readable -= thisread;
      
      uart_read_blocking(uart1, uartbuff, thisread);
      if (muteAudio) {
        for (int i=0; i<thisread; i++) {
          wtsynth_Sysex(uartbuff[i]);
        }
      } else {
        wtsynth_HandleMidiBlock(uartbuff, thisread);
      }
    }
  }
  
	wtsynth_Kill();
  reset_usb_boot(0, 0);
	return 0;
}

