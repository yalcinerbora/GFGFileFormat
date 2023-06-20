#include "GFGConversion.h"
#include "half.hpp"
#include <cassert>

static_assert(sizeof(half_float::half) == 2, "Half Size is not 16 bit");

// Simple Utility Cross
static auto GFGCrossProduct = [] (float out[3], const float a[3], const float b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
};

uint16_t GFGConversions::FloatToHalf(float f)
{
	half_float::half result(f);
	uint16_t out;
	std::memcpy(&out, &result, sizeof(uint16_t));
	return out;
}

uint16_t GFGConversions::DoubleToHalf(double d)
{

	half_float::half result(static_cast<float>(d));
	uint16_t out;
	std::memcpy(&out, &result, sizeof(half_float::half));
	return out;
}

uint8_t GFGConversions::DoubleToUnorm8(double d)
{
	double clamped = d <= 0.0 ? 0.0 : d >= 1.0 ? 1.0 : d;
	return static_cast<uint8_t>(clamped * std::numeric_limits<uint8_t>::max());
}

uint16_t GFGConversions::DoubleToUnorm16(double d)
{
	double clamped = d <= 0.0 ? 0.0 : d >= 1.0 ? 1.0 : d;
	return static_cast<uint16_t>(clamped * std::numeric_limits<uint16_t>::max());
}

uint32_t GFGConversions::DoubleToUnorm32(double d)
{
	double clamped = d <= 0.0 ? 0.0 : d >= 1.0 ? 1.0 : d;
	return static_cast<uint32_t>(clamped * std::numeric_limits<uint32_t>::max());
}

int8_t GFGConversions::DoubleToNorm8(double d)
{
	double clamped = d <= -1.0 ? -1.0 : d >= 1.0 ? 1.0 : d;
	return static_cast<int8_t>(clamped * std::numeric_limits<int8_t>::max());
}

int16_t GFGConversions::DoubleToNorm16(double d)
{
	double clamped = d <= -1.0 ? -1.0 : d >= 1.0 ? 1.0 : d;
	return static_cast<int16_t>(clamped * std::numeric_limits<int16_t>::max());
}

int32_t GFGConversions::DoubleToNorm32(double d)
{
	double clamped = d <= -1.0 ? -1.0 : d >= 1.0 ? 1.0 : d;
	return static_cast<int32_t>(clamped * std::numeric_limits<int32_t>::max());
}

uint8_t GFGConversions::FloatToUnorm8(float f)
{
	float clamped = f <= 0.0f ? 0.0f : f >= 1.0f ? 1.0f : f;
	return static_cast<uint8_t>(clamped * std::numeric_limits<uint8_t>::max());
}

uint16_t GFGConversions::FloatToUnorm16(float f)
{
	float clamped = f <= 0.0f ? 0.0f : f >= 1.0f ? 1.0f : f;
	return static_cast<uint16_t>(clamped * std::numeric_limits<uint16_t>::max());
}

uint32_t GFGConversions::FloatToUnorm32(float f)
{
	float clamped = f <= 0.0f ? 0.0f : f >= 1.0f ? 1.0f : f;
	return static_cast<uint32_t>(clamped * static_cast<float>(std::numeric_limits<uint32_t>::max()));
}

int8_t GFGConversions::FloatToNorm8(float f)
{
	float clamped = f <= -1.0f ? -1.0f : f >= 1.0f ? 1.0f : f;
	return static_cast<int8_t>(clamped * std::numeric_limits<int8_t>::max());
}

int16_t GFGConversions::FloatToNorm16(float f)
{
	float clamped = f <= -1.0f ? -1.0f : f >= 1.0f ? 1.0f : f;
	return static_cast<int8_t>(clamped * std::numeric_limits<int8_t>::max());
}

int32_t GFGConversions::FloatToNorm32(float f)
{
	float clamped = f <= -1.0f ? -1.0f : f >= 1.0f ? 1.0f : f;
	return static_cast<int8_t>(clamped * std::numeric_limits<int8_t>::max());
}

uint32_t GFGConversions::IntsToInt2_10_10_10(const int values[4])
{
	uint32_t result = 0;
	result |= values[3] << 30;
	result |= values[2] << 20;
	result |= values[1] << 10;
	result |= values[0] << 0;
	return result;
}

uint32_t GFGConversions::FloatsToInt2_10_10_10(const float values[4])
{
	static unsigned int max10BitValueSigned = 0x1FF;
	uint32_t result = 0;
	result |= static_cast<uint32_t>(values[3] <= -1.0 ? -1.0 : values[3] >= 1.0 ? 1.0 : values[3] * 3) << 30;
	result |= static_cast<uint32_t>(values[2] <= -1.0 ? -1.0 : values[2] >= 1.0 ? 1.0 : values[2] * max10BitValueSigned) << 20;
	result |= static_cast<uint32_t>(values[1] <= -1.0 ? -1.0 : values[1] >= 1.0 ? 1.0 : values[1] * max10BitValueSigned) << 10;
	result |= static_cast<uint32_t>(values[0] <= -1.0 ? -1.0 : values[0] >= 1.0 ? 1.0 : values[0] * max10BitValueSigned) << 0;
	return result;
}

uint32_t GFGConversions::DoublesToInt2_10_10_10(const double values[4])
{
	static unsigned int max10BitValueSigned = 0x1FF;
	uint32_t result = 0;
	result |= static_cast<uint32_t>(values[3] <= -1.0 ? -1.0 : values[3] >= 1.0 ? 1.0 : values[3] * 0x3) << 30;
	result |= static_cast<uint32_t>(values[2] <= -1.0 ? -1.0 : values[2] >= 1.0 ? 1.0 : values[2] * max10BitValueSigned) << 20;
	result |= static_cast<uint32_t>(values[1] <= -1.0 ? -1.0 : values[1] >= 1.0 ? 1.0 : values[1] * max10BitValueSigned) << 10;
	result |= static_cast<uint32_t>(values[0] <= -1.0 ? -1.0 : values[0] >= 1.0 ? 1.0 : values[0] * max10BitValueSigned) << 0;
	return result;
}

uint32_t GFGConversions::UIntsToUInt2_10_10_10(const unsigned int values[4])
{
	uint32_t result = 0;
	result |= values[3] << 30;
	result |= values[2] << 20;
	result |= values[1] << 10;
	result |= values[0] << 0;
	return result;
}

uint32_t GFGConversions::FloatsToUInt2_10_10_10(const float values[4])
{
	static unsigned int max10BitValue = 0x3FF;
	uint32_t result = 0;
	result |= static_cast<uint32_t>(values[3] <= 0.0 ? 0.0 : values[3] >= 1.0 ? 1.0 : values[3] * 1) << 30;
	result |= static_cast<uint32_t>(values[2] <= 0.0 ? 0.0 : values[2] >= 1.0 ? 1.0 : values[2] * max10BitValue) << 20;
	result |= static_cast<uint32_t>(values[1] <= 0.0 ? 0.0 : values[1] >= 1.0 ? 1.0 : values[1] * max10BitValue) << 10;
	result |= static_cast<uint32_t>(values[0] <= 0.0 ? 0.0 : values[0] >= 1.0 ? 1.0 : values[0] * max10BitValue) << 0;
	return result;
}

uint32_t GFGConversions::DoublesToUInt2_10_10_10(const double values[4])
{
	static unsigned int max10BitValue = 0x3FF;
	uint32_t result = 0;
	result |= static_cast<uint32_t>(values[3] <= 0.0 ? 0.0 : values[3] >= 1.0 ? 1.0 : values[3] * 3) << 30;
	result |= static_cast<uint32_t>(values[2] <= 0.0 ? 0.0 : values[2] >= 1.0 ? 1.0 : values[2] * max10BitValue) << 20;
	result |= static_cast<uint32_t>(values[1] <= 0.0 ? 0.0 : values[1] >= 1.0 ? 1.0 : values[1] * max10BitValue) << 10;
	result |= static_cast<uint32_t>(values[0] <= 0.0 ? 0.0 : values[0] >= 1.0 ? 1.0 : values[0] * max10BitValue) << 0;
	return result;
}

uint32_t GFGConversions::FloatsToUInt10F_11F_11F(const float values[3])
{
	// 10 bit floating point has 5bit exponent 5 bit mantissa NO sign bit
	// 11 bit floating point has 5bit exponent 6 bit mantissa NO sign bit
	// TODO Implement later
	auto floatTo10F = [] (const float) -> uint32_t
	{
		assert(false);
		return 0;
	};

	auto floatTo11F = [] (const float) -> uint32_t
	{
		assert(false);
		return 0;
	};

	uint32_t f1, f2, f3;
	uint32_t result;
	f1 = floatTo11F(values[0]);
	f2 = floatTo11F(values[1]);
	f3 = floatTo10F(values[2]);

	result = 0xFFFFFFFF;
	result &= f3 << 22;
	result &= f2 << 11;
	result &= f1 << 0;

	return result;
}

uint32_t GFGConversions::DoublesToUInt10F_11F_11F(const double values[3])
{
	// 10 bit floating point has 5bit exponent 5 bit mantissa NO sign bit
	// 11 bit floating point has 5bit exponent 6 bit mantissa NO sign bit
	// TODO Implement later
	auto floatTo10F = [] (const double) -> uint32_t
	{
		assert(false);
		return 0;
	};

	auto floatTo11F = [] (const double) -> uint32_t
	{
		assert(false);
		return 0;
	};

	uint32_t f1, f2, f3;
	uint32_t result;
	f1 = floatTo11F(values[0]);
	f2 = floatTo11F(values[1]);
	f3 = floatTo10F(values[2]);

	result = 0xFFFFFFFF;
	result &= f3 << 22;
	result &= f2 << 11;
	result &= f1 << 0;

	return result;
}

uint32_t GFGConversions::FloatsToCustom_1_15N_16N(const float values[3])
{
	// Value needs to be a normalized value (vector)
	static unsigned int max16bitSigned = 0x7FFF;
	static unsigned int max15bitSigned = 0x3FFF;

	uint32_t sign;
	std::memcpy(&sign, (values + 2), sizeof(float));
	sign = sign >> 31;

	uint32_t y = static_cast<uint32_t>(values[1] <= -1.0f ? -1.0f : values[1] >= 1.0f ? 1.0f : values[1] * max15bitSigned);
	uint32_t x = static_cast<uint32_t>(values[0] <= -1.0f ? -1.0f : values[0] >= 1.0f ? 1.0f : values[0] * max16bitSigned);

	uint32_t result = 0;
	result |= sign << 31;
	result |= y << 16;
	result |= x << 0;
	return result;
}

uint32_t GFGConversions::DoublesToCustom_1_15N_16N(const double values[3])
{
	// Value needs to be a normalized value (vector)
	static unsigned int max16bitSigned = 0x7FFF;
	static unsigned int max15bitSigned = 0x3FFF;

	uint64_t sign;
	std::memcpy(&sign, (values + 2), sizeof(double));
	sign = sign >> 63;

	uint32_t y = static_cast<uint32_t>(values[1] <= -1.0 ? -1.0 : values[1] >= 1.0 ? 1.0 : values[1] * max15bitSigned);
	uint32_t x = static_cast<uint32_t>(values[0] <= -1.0 ? -1.0 : values[0] >= 1.0 ? 1.0 : values[0] * max16bitSigned);

	uint32_t result = 0;
	result |= sign << 31;
	result |= y << 16;
	result |= x << 0;
	return result;
}

void GFGConversions::FloatsToCustom_Tang_H_2N(uint32_t result[3],
											  const float normal[3],
											  const float tangent[3],
											  const float binormal[3])
{
	// Direct Copy First 2 Result
	std::memcpy(result, tangent, sizeof(float) * 2);

	// Tangent Z sign
	unsigned int tangZsign;
	std::memcpy(&tangZsign, &tangent[2], sizeof(float));
	tangZsign = tangZsign >> 31;

	// Binormal Headedness
	// Tangent Z sign
	unsigned int binormHeadedness;

	float outBinorm[3];
	GFGCrossProduct(outBinorm, normal, tangent);
	if(outBinorm[0] == binormal[0] &&
	   outBinorm[1] == binormal[1] &&
	   outBinorm[2] == binormal[2])
	{
		binormHeadedness = 0x0000;
	}
	else
	{
		binormHeadedness = 0x0001;
	}

	std::memcpy(reinterpret_cast<uint8_t*>(&result[2]), &tangZsign, sizeof(uint16_t));
	std::memcpy(reinterpret_cast<uint8_t*>(&result[2]) + sizeof(uint16_t), &binormHeadedness, sizeof(uint16_t));
}

void GFGConversions::DoublesToCustom_Tang_H_2N(uint32_t result[3],
											  const double normal[3],
											  const double tangent[3],
											  const double binormal[3])
{
	// Direct Copy First 2 Result
	float normalFloat[3];
	float tangentFloat[3];
	float binormalFloat[3];

	normalFloat[0] = static_cast<float>(normal[0]);
	normalFloat[1] = static_cast<float>(normal[1]);
	normalFloat[2] = static_cast<float>(normal[2]);

	tangentFloat[0] = static_cast<float>(tangent[0]);
	tangentFloat[1] = static_cast<float>(tangent[1]);
	tangentFloat[2] = static_cast<float>(tangent[2]);

	binormalFloat[0] = static_cast<float>(binormal[0]);
	binormalFloat[1] = static_cast<float>(binormal[1]);
	binormalFloat[2] = static_cast<float>(binormal[2]);

	FloatsToCustom_Tang_H_2N(result, normalFloat, tangentFloat, binormalFloat);
}

// Multi Weight
void GFGConversions::DoubleToUnorm16_2_4(uint8_t dataOut[],
										 size_t dataCapacity,
										 const double data[],
										 unsigned int maxWeightInfluence)
{
	assert(dataCapacity >= sizeof(uint16_t) * maxWeightInfluence);
	GFGConversions::DoubleToUnorm16V(dataOut, dataCapacity, data, maxWeightInfluence);
}

void GFGConversions::DoubleToUnorm8_4_4(uint8_t dataOut[],
										size_t dataCapacity,
										const double data[],
										unsigned int maxWeightInfluence)
{
	assert(dataCapacity >= sizeof(uint8_t) * maxWeightInfluence);
	GFGConversions::DoubleToUnorm8V(dataOut, dataCapacity, data, maxWeightInfluence);
}

void GFGConversions::UIntToUInt16_2_4(uint8_t dataOut[],
									  size_t dataCapacity,
									  const unsigned int data[],
									  unsigned int maxWeightInfluence)
{
	assert(dataCapacity >= sizeof(uint16_t) * maxWeightInfluence);
	for(unsigned int i = 0; i < maxWeightInfluence; i++)
	{
		uint16_t temp = static_cast<uint16_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(uint16_t), &temp, sizeof(uint16_t));
	}
}

void GFGConversions::UIntToUInt8_4_4(uint8_t dataOut[],
									 size_t dataCapacity,
									 const unsigned int data[],
									 unsigned int maxWeightInfluence)
{
	assert(dataCapacity >= sizeof(uint8_t) * maxWeightInfluence);
	for(unsigned int i = 0; i < maxWeightInfluence; i++)
	{
		uint8_t temp = static_cast<uint8_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(uint8_t), &temp, sizeof(uint8_t));
	}
}

void GFGConversions::DoubleToHalfV(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	uint16_t temp;
	assert(dataCapacity >= sizeof(half_float::half) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToHalf(data[i]);
		std::memcpy(dataOut + i * sizeof(uint16_t), &temp, sizeof(uint16_t));
	}
}

void GFGConversions::DoubleToFloatV(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	float temp;
	assert(dataCapacity >= sizeof(float) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = static_cast<float>(data[i]);
		std::memcpy(dataOut + i * sizeof(float), &temp, sizeof(float));
	}
}

void GFGConversions::DoubleToQuadV(uint8_t[], size_t, const double[], size_t)
{
	assert(false);
}

void GFGConversions::DoubleToUnorm8V(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	uint8_t temp;
	assert(dataCapacity >= sizeof(uint8_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToUnorm8(data[i]);
		std::memcpy(dataOut + i * sizeof(uint8_t), &temp, sizeof(uint8_t));
	}
}

void GFGConversions::DoubleToUnorm16V(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	uint16_t temp;
	assert(dataCapacity >= sizeof(uint16_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToUnorm16(data[i]);
		std::memcpy(dataOut + i * sizeof(uint16_t), &temp, sizeof(uint16_t));
	}
}

void GFGConversions::DoubleToUnorm32V(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	uint32_t temp;
	assert(dataCapacity >= sizeof(uint32_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToUnorm32(data[i]);
		std::memcpy(dataOut + i * sizeof(uint32_t), &temp, sizeof(uint32_t));
	}
}

void GFGConversions::DoubleToNorm8V(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	int8_t temp;
	assert(dataCapacity >= sizeof(int8_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToNorm8(data[i]);
		std::memcpy(dataOut + i * sizeof(int8_t), &temp, sizeof(int8_t));
	}
}

void GFGConversions::DoubleToNorm16V(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	int16_t temp;
	assert(dataCapacity >= sizeof(int16_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToNorm16(data[i]);
		std::memcpy(dataOut + i * sizeof(int16_t), &temp, sizeof(int16_t));
	}
}

void GFGConversions::DoubleToNorm32V(uint8_t dataOut[], size_t dataCapacity, const double data[], size_t dataAmount)
{
	int32_t temp;
	assert(dataCapacity >= sizeof(int32_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		temp = GFGConversions::DoubleToNorm32(data[i]);
		std::memcpy(dataOut + i * sizeof(int32_t), &temp, sizeof(int32_t));
	}
}

void GFGConversions::UIntToUInt8V(uint8_t dataOut[], size_t dataCapacity, const unsigned int data[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint8_t) * dataAmount);
	for(size_t i = 0; i < dataAmount; i++)
	{
		uint8_t temp = static_cast<uint8_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(uint8_t), &temp, sizeof(uint8_t));
	}
}

void GFGConversions::UIntToUInt16V(uint8_t dataOut[], size_t dataCapacity, const unsigned int data[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint16_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		uint16_t temp = static_cast<uint16_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(uint16_t), &temp, sizeof(uint16_t));
	}
}

void GFGConversions::UIntToUInt32V(uint8_t dataOut[], size_t dataCapacity, const unsigned int data[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint32_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		uint32_t temp = static_cast<uint32_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(uint32_t), &temp, sizeof(uint32_t));
	}
}

void GFGConversions::IntToInt8V(uint8_t dataOut[], size_t dataCapacity, const int data[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int8_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int8_t temp = static_cast<int8_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(int8_t), &temp, sizeof(int8_t));
	}
}

void GFGConversions::IntToInt16V(uint8_t dataOut[], size_t dataCapacity, const int data[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int16_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int16_t temp = static_cast<int16_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(int16_t), &temp, sizeof(int16_t));
	}
}

void GFGConversions::IntToInt32V(uint8_t dataOut[], size_t dataCapacity, const int data[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int32_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int32_t temp = static_cast<int32_t>(data[i]);
		std::memcpy(dataOut + i * sizeof(int32_t), &temp, sizeof(int32_t));
	}
}

void GFGConversions::HalfToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(half_float::half) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		half_float::half half;
		std::memcpy(&half, dataIn + i * sizeof(half_float::half), sizeof(half_float::half));
		dataOut[i] = static_cast<double>(static_cast<float>(half));
	}
}

void GFGConversions::FloatToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(float) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		float temp;
		std::memcpy(&temp, dataIn + i * sizeof(float), sizeof(float));
		dataOut[i] = temp;
	}
}

void GFGConversions::QuadToDoubleV(double[], size_t, const uint8_t[], size_t)
{
	assert(false);
}

void GFGConversions::UNorm8ToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint8_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		uint8_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(uint8_t), sizeof(uint8_t));
		dataOut[i] = static_cast<double>(temp) / std::numeric_limits<uint8_t>::max();
	}
}

void GFGConversions::UNorm16ToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint16_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		uint16_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(uint16_t), sizeof(uint16_t));
		dataOut[i] = static_cast<double>(temp) / std::numeric_limits<uint16_t>::max();
	}
}

void GFGConversions::UNorm32ToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint32_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		uint32_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(uint32_t), sizeof(uint32_t));
		dataOut[i] = static_cast<double>(temp) / std::numeric_limits<uint32_t>::max();
	}
}

void GFGConversions::Norm8ToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int8_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int8_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(int8_t), sizeof(int8_t));
		dataOut[i] = static_cast<double>(temp) / std::numeric_limits<int8_t>::max();
	}
}

void GFGConversions::Norm16ToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int16_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int16_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(int16_t), sizeof(int16_t));
		dataOut[i] = static_cast<double>(temp) / std::numeric_limits<int16_t>::max();
	}
}

void GFGConversions::Norm32ToDoubleV(double dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int32_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int32_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(int32_t), sizeof(int32_t));
		dataOut[i] = static_cast<double>(temp) / std::numeric_limits<int32_t>::max();
	}
}

void GFGConversions::UInt8ToUIntV(unsigned int dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(uint8_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int8_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(int8_t), sizeof(int8_t));
		dataOut[i] = static_cast<unsigned int>(temp);
	}
}

void GFGConversions::UInt16ToUIntV(unsigned int dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int16_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int16_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(int16_t), sizeof(int16_t));
		dataOut[i] = static_cast<unsigned int>(temp);
	}
}

void GFGConversions::UInt32ToUIntV(unsigned int dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount)
{
	assert(dataCapacity >= sizeof(int32_t) * dataAmount);
	for(unsigned int i = 0; i < dataAmount; i++)
	{
		int32_t temp;
		std::memcpy(&temp, dataIn + i * sizeof(int32_t), sizeof(int32_t));
		dataOut[i] = static_cast<unsigned int>(temp);
	}
}

// TODO Implement
//static void				Int8ToIntV(int dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount);
//static void				Int16ToIntV(int dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount);
//static void				Int32ToIntV(int dataOut[], size_t dataCapacity, const uint8_t dataIn[], size_t dataAmount);

void GFGConversions::Int2_10_10_10ToInts(int dataOut[4], uint32_t data)
{
	int32_t temp = data;
	temp = (temp >> 30) & 0x2;
	dataOut[3] = static_cast<int>(temp);
	temp = (temp >> 20) & 0x3FF;
	dataOut[2] = static_cast<int>(temp);
	temp = (temp >> 10) & 0x3FF;
	dataOut[1] = static_cast<int>(temp);
	temp = (temp >> 0) & 0x3FF;
	dataOut[0] = static_cast<int>(temp);
}

void GFGConversions::Int2_10_10_10ToFloats(float dataOut[4], uint32_t data)
{
	static unsigned int max10BitValue = 0x3FF;
	static unsigned int max10BitValueSigned = 0x1FF;
	int32_t temp = data;
	temp = (temp >> 30) & 0x2;
	dataOut[3] = static_cast<float>(temp) / 0x2;
	temp = (temp >> 20) & 0x3FF;
	dataOut[2] = static_cast<float>(temp) / max10BitValue - max10BitValueSigned;
	temp = (temp >> 10) & 0x3FF;
	dataOut[1] = static_cast<float>(temp) / max10BitValue - max10BitValueSigned;
	temp = (temp >> 0) & 0x3FF;
	dataOut[0] = static_cast<float>(temp) / max10BitValue - max10BitValueSigned;
}

void GFGConversions::Int2_10_10_10ToDoubles(double dataOut[4], uint32_t data)
{
	static unsigned int max10BitValue = 0x3FF;
	static unsigned int max10BitValueSigned = 0x1FF;
	int32_t temp = data;
	temp = (temp >> 30) & 0x2;
	dataOut[3] = static_cast<double>(temp) / 0x2;
	temp = (temp >> 20) & 0x3FF;
	dataOut[2] = static_cast<double>(temp) / max10BitValue - max10BitValueSigned;
	temp = (temp >> 10) & 0x3FF;
	dataOut[1] = static_cast<double>(temp) / max10BitValue - max10BitValueSigned;
	temp = (temp >> 0) & 0x3FF;
	dataOut[0] = static_cast<double>(temp) / max10BitValue - max10BitValueSigned;
}

void GFGConversions::UInt2_10_10_10ToUInts(unsigned int dataOut[4], uint32_t data)
{
	uint32_t temp = data;
	temp = (temp >> 30) & 0x2;
	dataOut[3] = static_cast<unsigned int>(temp);
	temp = (temp >> 20) & 0x3FF;
	dataOut[2] = static_cast<unsigned int>(temp);
	temp = (temp >> 10) & 0x3FF;
	dataOut[1] = static_cast<unsigned int>(temp);
	temp = (temp >> 0) & 0x3FF;
	dataOut[0] = static_cast<unsigned int>(temp);
}

void GFGConversions::UInt2_10_10_10ToFloats(float dataOut[4], uint32_t data)
{
	static unsigned int max10BitValue = 0x3FF;
	static unsigned int max10BitValueSigned = 0x1FF;
	int32_t temp = data;
	temp = (temp >> 30) & 0x2;
	dataOut[3] = static_cast<float>(temp) / 0x2;
	temp = (temp >> 20) & 0x3FF;
	dataOut[2] = static_cast<float>(temp) / max10BitValue - max10BitValueSigned;
	temp = (temp >> 10) & 0x3FF;
	dataOut[1] = static_cast<float>(temp) / max10BitValue - max10BitValueSigned;
	temp = (temp >> 0) & 0x3FF;
	dataOut[0] = static_cast<float>(temp) / max10BitValue - max10BitValueSigned;
}

void GFGConversions::UInt2_10_10_10ToDoubles(double dataOut[4], uint32_t data)
{
	static unsigned int max10BitValue = 0x3FF;
	int32_t temp = data;
	temp = (temp >> 30) & 0x2;
	dataOut[3] = static_cast<double>(temp) / 0x2;
	temp = (temp >> 20) & max10BitValue;
	dataOut[2] = static_cast<double>(temp) / 0x3FF;
	temp = (temp >> 10) & max10BitValue;
	dataOut[1] = static_cast<double>(temp) / 0x3FF;
	temp = (temp >> 0) & max10BitValue;
	dataOut[0] = static_cast<double>(temp) / 0x3FF;
}

void GFGConversions::UInt10F_11F_11FToFloats(float[3], uint32_t)
{
	// TODO Implement Later
	assert(false);
}

void GFGConversions::UInt10F_11F_11FToDoubles(double[3], uint32_t)
{
	// TODO Implement Later
	assert(false);
}

void GFGConversions::Custom_1_15N_16NToFloats(float dataOut[3], uint32_t data)
{
	// Value needs to be a normalized value (vector)
	static unsigned int max16bitSigned = 0x7FFF;
	static unsigned int max15bitSigned = 0x3FFF;

	int sign = data >> 31 == 0 ? 1 : -1;
	dataOut[1] = static_cast<float>((data >> 16) & 0x0000FFFF) / max16bitSigned;
	dataOut[0] = static_cast<float>((data >> 0) & 0x0000FFFF) / max15bitSigned;
	dataOut[2] = sign * sqrt(1.0f - (dataOut[0] * dataOut[0] + dataOut[1] * dataOut[1]));
}

void GFGConversions::Custom_1_15N_16NToDoubles(double dataOut[3], uint32_t data)
{
	// Value needs to be a normalized value (vector)
	static unsigned int max16bitSigned = 0x7FFF;
	static unsigned int max15bitSigned = 0x3FFF;

	int sign = data >> 31 == 0 ? 1 : -1;
	dataOut[1] = static_cast<double>((data >> 16) & 0x0000FFFF) / max16bitSigned;
	dataOut[0] = static_cast<double>((data >> 0) & 0x0000FFFF) / max15bitSigned;
	dataOut[2] = sign * sqrt(1.0f - (dataOut[0] * dataOut[0] + dataOut[1] * dataOut[1]));
}

void GFGConversions::Unorm16_2_4ToDoubles(double dataOut[],
										  unsigned int& maxWeightInfluence,
										  size_t dataCapacity,
										  const uint8_t data[])
{
	assert(dataCapacity >= sizeof(uint16_t) * 8);
	GFGConversions::UNorm16ToDoubleV(dataOut, dataCapacity, data, maxWeightInfluence);
	maxWeightInfluence = 8;
}

void GFGConversions::Unorm8_4_4ToDoubles(double dataOut[],
										 unsigned int& maxWeightInfluence,
										 size_t dataCapacity,
										 const uint8_t data[])
{
	assert(dataCapacity >= sizeof(uint8_t) * 16);
	GFGConversions::UNorm8ToDoubleV(dataOut, dataCapacity, data, maxWeightInfluence);
	maxWeightInfluence = 16;
}

void GFGConversions::UInt16_2_4ToUInts(unsigned int dataOut[],
									   unsigned int& maxWeightInfluence,
									   size_t dataCapacity,
									   const uint8_t data[])
{
	assert(dataCapacity >= sizeof(uint16_t) * 8);
	for(unsigned int i = 0; i < 8; i++)
	{
		uint16_t temp;
		std::memcpy(&temp, &data[i * (sizeof(uint16_t) / sizeof(uint8_t))],
					sizeof(uint16_t));
		dataOut[i] = static_cast<unsigned int>(temp);
	}
	maxWeightInfluence = 8;
}

void GFGConversions::UInt8_4_4ToUInts(unsigned int dataOut[],
									  unsigned int& maxWeightInfluence,
									  size_t dataCapacity,
									  const uint8_t data[])
{
	assert(dataCapacity >= sizeof(uint8_t) * 16);
	for(unsigned int i = 0; i < 8; i++)
	{
		dataOut[i] = static_cast<unsigned int>(data[i]);
	}
	maxWeightInfluence = 16;
}
