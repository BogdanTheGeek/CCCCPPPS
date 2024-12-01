/**
 * @file
 * @brief Header file for CH32V003 flash memory management.
 * @author Tal G and recallmenot
 *
 * This header file contains the interface for managing the flash memory on the CH32V003 microcontroller.
 * It provides functions for storing and retrieving non-volatile settings in the flash memory.
 *
 * @note Integrated flash has limited write cycles (10K as per datasheet) and will wear out quickly without proper care.
 * For frequent writes, consider using external flash chips for increased durability.
 *
 * @note CH32V003 has 16K of flash memory, which is divided into 256 pages of 64 bytes each.
 *
 * @mainpage Flash Memory Management for CH32V003
 *
 * This library allows for the storage and retrieval of non-volatile settings in the CH32V003 microcontroller's flash memory.
 * It provides functions to write, read, and erase flash memory pages and manage option bytes.
 *
 * @section usage Usage
 * To store variables in the main flash, reserve one or more pages at the end of the main flash for storing non-volatile variables.
 * Follow the provided guidelines in overrides.ld and your Makefile to set the flash length.
 *
 * During the boot phase, call flash_set_latency() once.
 *
 * Read operations (getter functions) can be performed at any time and do not require unlocking.
 *
 * To alter the main flash, follow these steps:
 * 1. Unlock the flash.
 * 2. Erase page(s) (unfortunately, you can't erase smaller than a page).
 * 3. Program all desired values to the pages.
 * 4. Lock the flash.
 *
 * To alter option bytes data1 and data0, follow these steps:
 * 1. Unlock the flash.
 * 2. Unlock option bytes.
 * 3. Write to option bytes; this will internally erase them and restore other option byte values.
 * 4. Lock the flash.
 *
 * @section technical_notes Technical Notes
 * - Trying to write to addresses outside the main flash will likely freeze the microcontroller, so ensure your code doesn't attempt it.
 * - Option bytes store data as IIIIIIII DDDDDDDD, with D as data (byte0) and I as the inverse of data (byte1).
 * - To write a byte (8 bits), the write needs to be 16 bits (uint16_t), with the upper 16 bits (I) replaced with the inverted bit pattern of D.
 * - Use provided functions for reading and writing data1 and data0 bytes or manipulate OB->Data1 yourself if you require raw speed.
 *
 * @section address_calculations Address Calculations
 * To calculate the address for storing variables in the main flash, use the following formula:
 * \f$ \text{address of byte nonvolatile}[n] = \text{FLASH_BASE} + \text{N_BYTES} + [n] \f$
 * where n is the offset from the start of your reserved nonvolatile storage.
 *
 * For example:
 * \f$ \text{nonvolatile}[15] = 0x08000000 + 16320 + 1 + 15 \f$
 *
 * The function flash_calculate_runtime_address(n) performs this calculation.
 *
 * @note It is suggested to calculate all non-volatile storage addresses at the beginning of the main and store them in variables.
 * Alternatively, use the FLASH_PRECALCULATE_NONVOLATILE_ADDR(n) preprocessor macro to define your addresses, allowing the math to be done at compile time.
 */
#ifndef CH32V003_FLASH_H
#define CH32V003_FLASH_H
#include "ch32v003fun.h"
#include <stdint.h> // for uintN_t type support
/**
 * @brief Calculate the runtime address for nonvolatile storage.
 *
 * This function calculates the address at runtime for nonvolatile storage.
 * The address can also be calculated at compile time using FLASH_PRECALCULATE_NONVOLATILE_ADDR(n).
 *
 * @param byte_number The offset byte number from the start of nonvolatile storage.
 * @return uint32_t The calculated address for the specified nonvolatile byte.
 */
static inline uint32_t flash_calculate_runtime_address(uint16_t byte_number);
/**
 * @brief Set the flash controller latency.
 *
 * This function sets the flash controller latency in accordance with the FUNCONF_SYSTEM_CORE_CLOCK speed.
 * It should be called during the boot phase.
 */
static inline void flash_set_latency();
/**
 * @brief Unlock the main flash for altering.
 *
 * This function unlocks the main flash memory to allow modifications such as erase or program operations.
 */
static inline void flash_unlock();
/**
 * @brief Unlock the option bytes for altering.
 *
 * This function unlocks the option bytes to allow modifications.
 * It should be used in conjunction with flash_unlock() when modifying option bytes.
 */
static inline void flash_unlock_option_bytes();
/**
 * @brief Lock the flash after modifications.
 *
 * This function locks the flash memory after completing modifications to prevent unintended writes.
 */
static inline void flash_lock();
/**
 * @brief Erase a 64-byte page in flash memory.
 *
 * This function erases a 64-byte page in the flash memory. Note that smaller erases are not possible on the CH32V003.
 *
 * @param start_addr The start address of the page to be erased.
 */
static inline void flash_erase_page(uint32_t start_addr);
/**
 * @brief Program 16 bits of data into flash memory.
 *
 * This function programs 16 bits of data into flash memory.
 * The flash memory must be unlocked before calling this function.
 *
 * @param addr The address where the data will be programmed.
 * @param data The 16-bit data to be programmed.
 */
static inline void flash_program_16(uint32_t addr, uint16_t data);
/**
 * @brief Program two 8-bit values into flash memory.
 *
 * This function programs two 8-bit values into flash memory.
 * It utilizes the flash_program_16 function for the actual programming.
 *
 * @param addr The address where the data will be programmed.
 * @param byte1 The first 8-bit value to be programmed.
 * @param byte0 The second 8-bit value to be programmed.
 */
static inline void flash_program_2x8_bits(uint32_t addr, uint8_t byte1, uint8_t byte0);
/**
 * @brief Program a float value into flash memory.
 *
 * This function programs a float value into flash memory by breaking it down into two 16-bit values.
 *
 * @param addr The address where the float value will be programmed.
 * @param value The float value to be programmed.
 */
static inline void flash_program_float_value(uint32_t addr, float value);
/**
 * @brief Read 16 bits of data from flash memory.
 *
 * This function reads 16 bits of data from the specified address in flash memory.
 *
 * @param addr The address from which the data will be read.
 * @return uint16_t The 16-bit data read from the specified address.
 */
static inline uint16_t flash_read_16_bits(uint32_t addr);
/**
 * @brief Read an 8-bit value from flash memory.
 *
 * This function reads an 8-bit value from the specified address in flash memory.
 *
 * @param addr The address from which the data will be read.
 * @return uint8_t The 8-bit data read from the specified address.
 */
static inline uint8_t flash_read_8_bits(uint32_t addr);
/**
 * @brief Read a float value from flash memory.
 *
 * This function reads a float value from the specified address in flash memory by combining two 16-bit values.
 *
 * @param addr The address from which the float value will be read.
 * @return float The float value read from the specified address.
 */
static inline float flash_read_float_value(uint32_t addr);
/**
 * @brief Write 16 bits of data to the option bytes.
 *
 * This function writes 16 bits of data to the option bytes while keeping other contents intact.
 * It involves backing up, erasing, and then writing the new data.
 *
 * @param data The 16-bit data to be written to the option bytes.
 */
static inline void flash_write_option_byte_16_bits(uint16_t data);
/**
 * @brief Write two 8-bit values to the option bytes.
 *
 * This function writes two 8-bit values to the option bytes.
 * It utilizes the flash_write_option_byte_16_bits function for the actual writing.
 *
 * @param data1 The first 8-bit value to be written.
 * @param data0 The second 8-bit value to be written.
 */
static inline void flash_write_option_byte_2x8_bits(uint8_t data1, uint8_t data0);
/**
 * @brief Read the USER option byte.
 *
 * This function reads the USER option byte from the option bytes area.
 *
 * @return uint8_t The USER option byte value.
 */
static inline uint8_t flash_read_option_byte_USER();
/**
 * @brief Read the RDPR option byte.
 *
 * This function reads the RDPR option byte from the option bytes area.
 *
 * @return uint8_t The RDPR option byte value.
 */
static inline uint8_t flash_read_option_byte_RDPR();
/**
 * @brief Read the WRPR1 option byte.
 *
 * This function reads the WRPR1 option byte from the option bytes area.
 *
 * @return uint8_t The WRPR1 option byte value.
 */
static inline uint8_t flash_read_option_byte_WRPR1();
/**
 * @brief Read the WRPR0 option byte.
 *
 * This function reads the WRPR0 option byte from the option bytes area.
 *
 * @return uint8_t The WRPR0 option byte value.
 */
static inline uint8_t flash_read_option_byte_WRPR0();
/**
 * @brief Read the DATA1 option byte.
 *
 * This function reads the DATA1 option byte from the option bytes area.
 *
 * @return uint8_t The DATA1 option byte value.
 */
static inline uint8_t flash_read_option_byte_DATA1();
/**
 * @brief Read the DATA0 option byte.
 *
 * This function reads the DATA0 option byte from the option bytes area.
 *
 * @return uint8_t The DATA0 option byte value.
 */
static inline uint8_t flash_read_option_byte_DATA0();
/**
 * @brief Read both DATA1 and DATA0 option bytes as a 16-bit value.
 *
 * This function reads both DATA1 and DATA0 option bytes from the option bytes area and combines them into a 16-bit value.
 *
 * @return uint16_t The combined 16-bit value of DATA1 and DATA0 option bytes.
 */
static inline uint16_t flash_read_option_byte_DATA_16();
// Internal Function Declarations
/**
 * @brief Check if the flash is currently busy.
 *
 * This function returns the operational status of the flash, indicating whether it is busy with an ongoing operation.
 *
 * @return uint8_t Non-zero if the flash is busy, zero otherwise.
 */
static inline uint8_t flash_is_busy();
/**
 * @brief Check if the last flash operation has completed.
 *
 * This function checks the status of the last flash operation, specifically if it has been completed.
 *
 * @return uint8_t Non-zero if the operation is completed, zero otherwise.
 */
static inline uint8_t flash_is_done();
/**
 * @brief Check for write protection error in flash.
 *
 * This function checks if a write protection error has occurred during the last flash operation.
 *
 * @return uint8_t Non-zero if a write protection error occurred, zero otherwise.
 */
static inline uint8_t flash_is_ERR_WRPRT();
/**
 * @brief Clear the flash operation done (EOP) bit.
 *
 * This function clears the End Of Programming (EOP) bit in the flash status register.
 */
static inline void flash_is_done_clear();
/**
 * @brief Wait until the flash is no longer busy.
 *
 * This function blocks execution until the flash memory is no longer busy with an ongoing operation.
 */
static inline void flash_wait_until_not_busy();
/**
 * @brief Wait until the last flash operation is done.
 *
 * This function blocks execution until the last flash operation is completed.
 */
static inline void flash_wait_until_done();
/**
 * @brief Verify that the first 8 bits are the inverse of the last 8 bits.
 *
 * This function is used to test if the first 8 bits of the input are the inverse of the last 8 bits, typically used for data validation.
 *
 * @param input 16-bit input data to be checked.
 * @return uint8_t Non-zero if the test passes, zero otherwise.
 */
static inline uint8_t flash_dechecksum(uint16_t input);
/**
 * @brief Erase the option byte.
 *
 * This function erases the option byte in flash memory.
 */
static inline void flash_OB_erase();
// Internal variables
extern char FLASH_LENGTH_OVERRIDE[]; // import from .ld, halal by https://sourceware.org/binutils/docs/ld/Source-Code-Reference.html
union float_uint32t
{
    float f;
    uint32_t u32;
};
union float_2xuint16t
{
    float f;
    uint16_t u16[2];
};
union uint16t_2xuint8t
{
    uint16_t u16;
    uint8_t u8[2];
};
// Preprocessor Macros
#define FLASH_VOLATILE_CAPACITY (FLASH_BASE - FLASH_LENGTH_OVERRIDE)
// use this to define main flash nonvolatile addresses at compile time!
#define FLASH_PRECALCULATE_NONVOLATILE_ADDR(n) FLASH_BASE + (uint32_t)(uintptr_t)(FLASH_LENGTH_OVERRIDE) + n
// Function Definitions
static inline uint32_t flash_calculate_runtime_address(uint16_t byte_number)
{
    // Calculate the non-volatile storage address by adding the base address of the flash,
    // the override length (to account for any offsets), and the specific byte number requested.
    return (FLASH_BASE + (uint16_t)(uintptr_t)FLASH_LENGTH_OVERRIDE + byte_number);
}
static inline void flash_set_latency()
{
// Check if the system core clock speed is less than or equal to 24 MHz.
#if FUNCONF_SYSTEM_CORE_CLOCK <= 24000000
    // If yes, set the flash latency to zero wait states.
    FLASH->ACTLR = FLASH_Latency_0;
#else
    // Otherwise, set the flash latency to one wait state.
    FLASH->ACTLR = FLASH_Latency_1;
#endif
}
static inline void flash_unlock()
{
    // Write the first key to the flash key register for unlocking.
    FLASH->KEYR = FLASH_KEY1;
    // Write the second key to completely unlock the flash.
    FLASH->KEYR = FLASH_KEY2;
}
static inline void flash_unlock_option_bytes()
{
    // Write the first key to the option bytes key register for unlocking.
    FLASH->OBKEYR = FLASH_KEY1;
    // Write the second key to completely unlock the option bytes.
    FLASH->OBKEYR = FLASH_KEY2;
}
static inline void flash_lock()
{
    // Set the lock bit in the flash control register to lock the flash.
    FLASH->CTLR |= FLASH_CTLR_LOCK;
}
static inline void flash_erase_page(uint32_t start_addr)
{
    // Check if the flash is locked.
    if (FLASH->CTLR & FLASH_CTLR_LOCK)
    {
        // If locked, exit the function.
        return;
    }
    // Wait until the flash is not busy before starting the erase operation.
    flash_wait_until_not_busy();
    // Set the page erase bit in the control register.
    FLASH->CTLR |= CR_PER_Set;
    // Set the address of the page to be erased.
    FLASH->ADDR = start_addr;
    // Start the erase operation.
    FLASH->CTLR |= CR_STRT_Set;
    // Wait until the flash is not busy again after the erase.
    flash_wait_until_not_busy();
    // Reset the page erase bit.
    FLASH->CTLR &= CR_PER_Reset;
}
static inline void flash_program_16(uint32_t addr, uint16_t data)
{
    // Check if the flash is locked.
    if (FLASH->CTLR & FLASH_CTLR_LOCK)
    {
        // If locked, exit the function.
        return;
    }
    // Wait until the flash is not busy before starting the program operation.
    flash_wait_until_not_busy();
    // Enable the flash programming by setting the PG bit in the control register.
    FLASH->CTLR |= CR_PG_Set;
    // Program the 16-bit data at the specified address.
    *(uint16_t *)(uintptr_t)addr = data;
    // Wait until the flash is not busy again after programming.
    flash_wait_until_not_busy();
    // Reset the PG bit to disable flash programming.
    FLASH->CTLR &= CR_PG_Reset;
}
static inline void flash_program_2x8_bits(uint32_t addr, uint8_t byte1, uint8_t byte0)
{
    // Combines two 8-bit values into a 16-bit value and programs it into flash at the specified address.
    flash_program_16(addr, (byte1 << 8) + byte0);
}
static inline void flash_program_float_value(uint32_t addr, float value)
{
    // A union is used to manipulate the float value as two 16-bit integers.
    union float_2xuint16t conv;
    conv.f = value; // Store the float value in the union.
    // Program the first 16 bits of the float.
    flash_program_16(addr, conv.u16[0]);
    // Program the second 16 bits of the float.
    flash_program_16(addr + 2, conv.u16[1]);
}
static inline uint16_t flash_read_16_bits(uint32_t addr)
{
    // Returns a 16-bit value from the specified flash memory address.
    return *(uint16_t *)(uintptr_t)addr;
}
static inline uint8_t flash_read_8_bits(uint32_t addr)
{
    // Returns an 8-bit value from the specified flash memory address.
    return *(uint8_t *)(uintptr_t)addr;
}
static inline float flash_read_float_value(uint32_t addr)
{
    // A union is used for reading a float value as two 16-bit integers.
    union float_2xuint16t conv;
    // Read the first 16 bits of the float.
    conv.u16[0] = ((uint16_t *)(uintptr_t)addr)[0];
    // Read the second 16 bits of the float.
    conv.u16[1] = ((uint16_t *)(uintptr_t)addr)[1];
    // Return the combined float value.
    return conv.f;
}
static inline void flash_write_option_byte_16_bits(uint16_t data)
{
    // Wait until the flash is not busy before starting any operation.
    flash_wait_until_not_busy();
    // Backup current option bytes data.
    uint16_t tmp_user = OB->USER;
    uint16_t tmp_rdpr = OB->RDPR;
    uint16_t tmp_wrpr1 = OB->WRPR1;
    uint16_t tmp_wrpr0 = OB->WRPR0;
    // Split the 16-bit data into two 8-bit values.
    uint16_t tmp_data1 = (data >> 8) & 0xFF;
    uint16_t tmp_data0 = data & 0xFF;
    // Erase the current option bytes.
    flash_OB_erase();
    // Enable option byte programming.
    FLASH->CTLR |= CR_OPTPG_Set;
    // Restore the backup data to option bytes.
    OB->USER = tmp_user;
    flash_wait_until_not_busy();
    OB->RDPR = tmp_rdpr;
    flash_wait_until_not_busy();
    OB->WRPR0 = tmp_wrpr0;
    flash_wait_until_not_busy();
    OB->WRPR1 = tmp_wrpr1;
    flash_wait_until_not_busy();
    // Write the new data to option bytes.
    OB->Data1 = tmp_data1; // High byte
    flash_wait_until_not_busy();
    OB->Data0 = tmp_data0; // Low byte
    flash_wait_until_not_busy();

    // Disable option byte programming.
    FLASH->CTLR &= CR_OPTPG_Reset;
}
static inline void flash_write_option_byte_2x8_bits(uint8_t data1, uint8_t data0)
{
    flash_write_option_byte_16_bits((data1 << 8) + data0);
}
static inline uint8_t flash_read_option_byte_USER()
{
    return flash_dechecksum(OB->USER);
}
static inline uint8_t flash_read_option_byte_RDPR()
{
    return flash_dechecksum(OB->RDPR);
}
static inline uint8_t flash_read_option_byte_WRPR1()
{
    return flash_dechecksum(OB->WRPR1);
}
static inline uint8_t flash_read_option_byte_WRPR0()
{
    return flash_dechecksum(OB->WRPR0);
}
static inline uint8_t flash_read_option_byte_DATA1()
{
    return flash_dechecksum(OB->Data1);
}
static inline uint8_t flash_read_option_byte_DATA0()
{
    return flash_dechecksum(OB->Data0);
}
static inline uint16_t flash_read_option_byte_DATA_16()
{
    return (flash_read_option_byte_DATA1() << 8) + flash_read_option_byte_DATA0();
}
// Internal Function Definitions.
static inline uint8_t flash_is_busy()
{
    return ((FLASH->STATR & FLASH_STATR_BSY) == FLASH_STATR_BSY);
}
static inline uint8_t flash_is_done()
{
    return ((FLASH->STATR & FLASH_STATR_EOP) == FLASH_STATR_EOP);
}
static inline uint8_t flash_is_ERR_WRPRT()
{
    return ((FLASH->STATR & FLASH_STATR_WRPRTERR) == FLASH_STATR_WRPRTERR);
}
static inline void flash_is_done_clear()
{
    FLASH->STATR |= FLASH_STATR_EOP;
}
static inline void flash_wait_until_not_busy()
{
    while (flash_is_busy())
    {
    }
}
static inline void flash_wait_until_done()
{
    while (flash_is_busy() || !flash_is_done())
    {
    }
    flash_is_done_clear();
}
static inline uint8_t flash_dechecksum(uint16_t input)
{
    // Extract the lower 8 bits (original data) from the input.
    uint8_t originalData = input & 0xFF; // 0xFF is equivalent to 0b11111111.
    // Extract the upper 8 bits (inverted data) and invert them for comparison.
    uint8_t invertedData = ~(input >> 8);
    // Check if the inverted data matches the original data.
    if (invertedData == originalData)
    {
        // If they match, return the original data.
        return originalData;
    }
    else
    {
        // If they don't match, return 0 to indicate a checksum error.
        return 0;
    }
}
static inline void flash_OB_erase()
{
    // Set the option byte erase bit in the flash control register.
    // This prepares the flash controller to erase the option bytes.
    FLASH->CTLR |= CR_OPTER_Set;
    // Set the start bit in the flash control register to begin the erase operation.
    FLASH->CTLR |= CR_STRT_Set;
    // Wait until the flash is not busy before proceeding.
    // This ensures that the erase operation is complete before any further actions.
    flash_wait_until_not_busy();
    // Reset the option byte erase bit to stop the erase operation.
    FLASH->CTLR &= CR_OPTER_Reset;
}
#endif // CH32V003_FLASH_H
