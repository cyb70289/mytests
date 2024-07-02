package bmi

import (
    "math/bits"
)

const lookupBits = 5

var pextTable = [1 << lookupBits][1 << lookupBits]uint8{
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
		0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
		0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
		0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02,
		0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01,
		0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
		0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x00, 0x01, 0x00,
		0x01, 0x02, 0x03, 0x02, 0x03, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03,
		0x02, 0x03, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x00, 0x00, 0x01,
		0x01, 0x02, 0x02, 0x03, 0x03, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02,
		0x03, 0x03, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02,
		0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02,
		0x03, 0x02, 0x03, 0x02, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03,
		0x03, 0x02, 0x02, 0x03, 0x03, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
		0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
		0x07, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x04, 0x05, 0x06, 0x07,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02,
		0x02, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
		0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05, 0x04,
		0x05, 0x06, 0x07, 0x06, 0x07, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03,
		0x02, 0x03, 0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
		0x05, 0x06, 0x06, 0x07, 0x07, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02,
		0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
		0x01, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03,
		0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
		0x01, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x02, 0x02,
		0x03, 0x03, 0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02,
		0x03, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x04, 0x05,
		0x06, 0x07, 0x04, 0x05, 0x06, 0x07, 0x04, 0x05, 0x06, 0x07,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03,
		0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x00, 0x01, 0x00,
		0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05, 0x04, 0x05, 0x06, 0x07,
		0x06, 0x07, 0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x00, 0x00, 0x01,
		0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
		0x07, 0x07, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02,
		0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
		0x0E, 0x0F, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02,
		0x03, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05, 0x04, 0x05, 0x04, 0x05,
		0x04, 0x05, 0x06, 0x07, 0x06, 0x07, 0x06, 0x07, 0x06, 0x07,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03,
		0x03, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x04, 0x04,
		0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
		0x07, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x08, 0x09,
		0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0C, 0x0D, 0x0E, 0x0F,
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02,
		0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05,
		0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07,
	},
	{
		0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05, 0x04,
		0x05, 0x06, 0x07, 0x06, 0x07, 0x08, 0x09, 0x08, 0x09, 0x0A, 0x0B,
		0x0A, 0x0B, 0x0C, 0x0D, 0x0C, 0x0D, 0x0E, 0x0F, 0x0E, 0x0F,
	},
	{
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
		0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A,
		0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F,
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
		0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	},
}

// software emulation of _pext_u64
func ExtractBitsGo(bitmap, selectBitmap uint64) uint64 {
	if selectBitmap == ^uint64(0) {
		return bitmap
	} else if selectBitmap == 0 {
		return 0
	}

	// fallback to lookup table method
	bitValue := uint64(0)
	bitLen := int(0)
	const lookupMask = uint64((uint(1) << lookupBits) - 1)

	for selectBitmap != 0 {
		maskLen := bits.OnesCount32(uint32(selectBitmap & lookupMask))
		value := pextTable[selectBitmap&lookupMask][bitmap&lookupMask]
		bitValue |= uint64(value) << bitLen
		bitLen += maskLen
		bitmap >>= lookupBits
		selectBitmap >>= lookupBits
	}
	return bitValue
}

func _extract_bits_neon(bitmap, selectBitmap uint64) (res uint64)