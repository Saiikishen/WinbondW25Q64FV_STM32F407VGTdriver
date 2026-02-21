# W25Q64FV SPI Flash Driver for STM32F407VGT

A lightweight, production-ready HAL-based driver for the **Winbond W25Q64FV**
64 Mbit (8 MB) SPI NOR Flash, targeting the **STM32F407VGT** microcontroller.
---

## Features

- Full Standard SPI support (Mode 0 / Mode 3)
- Read, page-program, and multi-page write with automatic page-boundary splitting
- Sector erase (4 KB), block erase (32 KB / 64 KB), and full chip erase
- JEDEC ID verification at initialisation
- 64-bit unique ID read
- Status register read/write
- Deep Power-Down and Wake-Up
- Software Reset (Enable Reset + Reset sequence)
- Erase/Program Suspend and Resume
- Timeout-guarded busy polling for all operations
- Re-entrant handle-based design (supports multiple flash chips on one bus)

---

> **Note:** Do **not** assign any pin as `SPI_NSS` in CubeMX.
> Configure it as a plain `GPIO_Output` with the user label `FLASH_CS`.

---

## STM32CubeMX Configuration

### SPI Settings

| Parameter           | Value                        |
|---------------------|------------------------------|
| Mode                | Full-Duplex Master           |
| Data Size           | 8 Bit                        |
| First Bit           | MSB First                    |
| CPOL                | Low                          |
| CPHA                | 1 Edge (Mode 0)              |
| NSS                 | Software                     |
| Baud Rate Prescaler | `/2` → 21 MHz (APB1 = 42 MHz)|

> SPI is on APB1 (max 42 MHz). The W25Q64FV supports up to 104 MHz,
> so any prescaler is safe. `/2` (21 MHz) is recommended for signal integrity.


---

## File Structure

```
├── Core/
│   ├── Inc/
│   │   ├── w25q64fv.h          # Flash driver header
│   └── Src/
│       ├── w25q64fv.c          # Flash driver implementation
│       ├── flash_example.c     # Live Expressions test demo
```

---

## Quick Start

### 1. Initialise

```c
#include "w25q64fv.h"

extern SPI_HandleTypeDef hspi2;

W25Q64FV_Handle flash;

// Call after MX_GPIO_Init() and MX_SPI2_Init()
W25Q64FV_Status status = W25Q64FV_Init(&flash, &hspi2,
                                         FLASH_CS_GPIO_Port,
                                         FLASH_CS_Pin);
// Returns W25Q64FV_OK if JEDEC ID matches 0xEF4017
```

### 2. Erase → Write → Read

```c
// Erase a 4 KB sector (must erase before writing)
W25Q64FV_SectorErase(&flash, 0x000000);

// Write data (handles page boundaries automatically)
uint8_t tx;
W25Q64FV_Write(&flash, 0x000000, tx, sizeof(tx));

// Read data back
uint8_t rx;
W25Q64FV_Read(&flash, 0x000000, rx, sizeof(rx));
```

### 3. Power Management

```c
W25Q64FV_PowerDown(&flash);   // ~1 µA deep sleep
// ... time passes ...
W25Q64FV_WakeUp(&flash);      // must call before any command
```

---

## API Reference

### Initialisation

| Function | Description |
|----------|-------------|
| `W25Q64FV_Init()` | Init handle, wake device, verify JEDEC ID |
| `W25Q64FV_VerifyID()` | Confirm JEDEC ID = `0xEF4017` |
| `W25Q64FV_ReadJEDECID()` | Read manufacturer + device ID bytes |
| `W25Q64FV_ReadUniqueID()` | Read 64-bit factory serial number |

### Status

| Function | Description |
|----------|-------------|
| `W25Q64FV_ReadSR1()` | Read Status Register 1 (BUSY, WEL, BP bits) |
| `W25Q64FV_ReadSR2()` | Read Status Register 2 (QE, LB, CMP bits) |
| `W25Q64FV_WriteSR()` | Write both status registers |
| `W25Q64FV_WaitBusy()` | Poll BUSY bit until clear or timeout |

### Read / Write

| Function | Description |
|----------|-------------|
| `W25Q64FV_Read()` | Fast Read from any address, any length |
| `W25Q64FV_PageProgram()` | Write 1–256 bytes within a single page |
| `W25Q64FV_Write()` | Write any length, auto-splits page boundaries |

### Erase

| Function | Erase Size | Typical Time |
|----------|-----------|--------------|
| `W25Q64FV_SectorErase()` | 4 KB | ~50–150 ms |
| `W25Q64FV_BlockErase32K()` | 32 KB | ~120–800 ms |
| `W25Q64FV_BlockErase64K()` | 64 KB | ~150–1000 ms |
| `W25Q64FV_ChipErase()` | 8 MB | ~40–200 s |

### Power & Control

| Function | Description |
|----------|-------------|
| `W25Q64FV_PowerDown()` | Enter Deep Power-Down (0xB9) |
| `W25Q64FV_WakeUp()` | Release from Deep Power-Down (0xAB) |
| `W25Q64FV_SoftReset()` | Issue Enable Reset (0x66) + Reset (0x99) |
| `W25Q64FV_EraseSuspend()` | Pause an active erase for a read |
| `W25Q64FV_EraseResume()` | Resume a suspended erase |

### Address Helpers (inline)

```c
W25Q64FV_PageToAddr(page)      // page number → byte address
W25Q64FV_SectorToAddr(sector)  // sector number → byte address
W25Q64FV_BlockToAddr(block)    // block number → byte address
W25Q64FV_AddrToPage(addr)      // byte address → page number
W25Q64FV_AddrToSector(addr)    // byte address → sector number
```

---

## Return Codes

| Code | Value | Meaning |
|------|-------|---------|
| `W25Q64FV_OK` | 0 | Success |
| `W25Q64FV_ERR` | 1 | HAL SPI error |
| `W25Q64FV_TIMEOUT` | 2 | Operation timed out |
| `W25Q64FV_BUSY` | 3 | Device busy |
| `W25Q64FV_PARAM_ERR` | 4 | Invalid parameter or out-of-range address |
| `W25Q64FV_ID_MISMATCH` | 5 | JEDEC ID does not match W25Q64FV |

---

## Memory Map

| Region | Address Range | Size | Description |
|--------|--------------|------|-------------|
| Total | `0x000000` – `0x7FFFFF` | 8 MB | Full array |
| Block | 128 × 64 KB | 64 KB each | Erase with `BlockErase64K` |
| Sector | 2048 × 4 KB | 4 KB each | Erase with `SectorErase` |
| Page | 32768 × 256 B | 256 B each | Program with `PageProgram` |

---

## Device Identification

```
JEDEC Response (command 0x9F):
  Byte 1: 0xEF       → Winbond manufacturer ID
  Byte 2: 0x40       → SPI NOR Flash memory type
  Byte 3: 0x17       → 64 Mbit (2^23 = 8,388,608 bytes)
```

> **QE bit note:** This driver uses Standard SPI only.
> If your chip returns `SR2 = 0x02` (QE=1), it is an `IQ` factory variant
> with Quad mode pre-enabled. This does **not** affect standard SPI operation.
> The WP and HOLD pin hardware functions are disabled on these parts.

---


## Important Rules

1. **Always erase before writing** — flash bits can only go `1→0`; erase resets them to `0xFF`
2. **Never write across a page boundary** in a single `PageProgram` call — use `W25Q64FV_Write()` which handles this automatically
3. **Call `WakeUp()` after `PowerDown()`** before issuing any command — the chip ignores all commands except `0xAB` in power-down mode
4. **Check BUSY before `SoftReset()`** — resetting during an active erase/program corrupts data
5. **`Init()` always calls `WakeUp()` first** — safe to call after any MCU reset regardless of previous flash state

---

## Tested On

| Component | Details |
|-----------|---------|
| MCU | STM32F407VGT6 |
| Flash | Winbond W25Q64FVSSIG (IQ variant, QE=1) |
| HAL Version | STM32F4 HAL v1.8.x |
| IDE | STM32CubeIDE |
| SPI Clock | 21 MHz (APB1 84 MHz / 4) |

---
