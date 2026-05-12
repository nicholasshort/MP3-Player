# MP3 Player

This MP3 Player is very simple. All I want is this: something I can bring with me on runs, hikes, or even long trips that won't run out of battery after a few hours of use. With wired headphones, this device can provide 24+ hours of music on a single charge (at least that's the goal!). 

| Top | Bottom |
| :---: | :---: |
| ![Top of the assembled board](images/board-top.jpeg) | ![Bottom edge of the assembled board](images/board-bottom.jpeg) |

## Status

- **Hardware:** Rev A manufactured and assembled by JLCPCB.
- **Firmware:** In progress!

## Features

- **MCU:** STM32F411CEU6 (ARM Cortex-M4F, 100 MHz, USB OTG FS)
- **Audio:** TI TAD5242 stereo DAC fed over I²S, 3.5 mm headphone jack output
- **Storage:** microSD card slot (SPI)
- **Power:**
  - USB-C input (USB 2.0, with USBLC6 ESD protection)
  - TI BQ24259 Li-ion battery charger
  - Dual TPS7A20 LDOs for clean analog/digital rails
- **User interface:** 4 tactile switches (PWR, MENU, PLAY, NEXT) plus two WS2812B-2020 addressable RGB LEDs
- **PCB:** 4-layer, 50 × 50 mm

## Repository layout

```
MP3-Player/
├── MP3-Player-PCB/          KiCad 8 project (hardware design)
│   ├── *.kicad_sch          Schematic sheets: MCU, DAC, Power, Peripherals
│   ├── MP3-Player.kicad_pcb 4-layer PCB layout
│   ├── MP3-Player_Footprint_Libraries.pretty/  Custom footprints
│   ├── MP3-Player_Symbol_Libraries.kicad_sym   Custom symbols
│   ├── 3D Models/           STEP/snapshot files used by the PCB
│   ├── Silkscreen Images/   Artwork used on the silkscreen
│   ├── Manufacturing/
│   │   ├── Gerber/          Gerbers + drill files (and zipped bundle)
│   │   ├── BOM/             JLCPCB BOM with LCSC part numbers
│   │   └── Assembly/        Component placement (CPL) file
│   └── LICENSE              CERN-OHL-P v2 (hardware)
├── MP3-Player-Firmware/     STM32CubeMX project
│   ├── MP3-Player-Firmware.ioc
│   └── LICENSE              MIT (firmware)
└── images/                  Board photos used in this README
```

## Building the hardware

The `Manufacturing/` folder contains everything needed for a JLCPCB order:

- `Gerber/Gerbers.zip` — upload to JLCPCB's PCB ordering page
- `BOM/MP3-Player.csv` — BOM with LCSC part numbers for assembly
- `Assembly/MP3-Player-all-pos.csv` — component placement file

PCB stack-up: 4 layers, 1.6 mm.

## Building the firmware

The firmware project is a STM32CubeIDE / CubeMX project. The `.ioc` configures the peripherals used on this board:

- **I²S1** — 48 kHz, 32-bit, master, to the TAD5242 DAC
- **SPI2** — microSD card
- **I²C1** — BQ24259 charger
- **USB OTG FS** — USB-C
- **TIM2 + DMA1** — WS2812B drive
- **ADC1** — battery / thermistor sense

The main firmware is yet to be written, but lots to come.

## Licence

This project uses separate licences for hardware and firmware, which is the convention for open-hardware projects:

- **Hardware** (`MP3-Player-PCB/`) — [CERN Open Hardware Licence Version 2 — Permissive](MP3-Player-PCB/LICENSE) (`CERN-OHL-P-2.0`). You may use, modify, manufacture, and distribute the design and derivative products, including for commercial purposes, provided notices are retained.
- **Firmware** (`MP3-Player-Firmware/`) — [MIT](MP3-Player-Firmware/LICENSE).

Both licences are permissive and disclaim warranty. The board has not been tested for compliance with any regulatory standards (FCC, CE, RoHS, etc.) — build at your own risk.
