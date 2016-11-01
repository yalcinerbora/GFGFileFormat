#include "GFGVertexElementTypes.h"
#include "GFGConversion.h"
#include <cassert>

bool GFGPosition::IsCompatible(GFGDataType t)
{
	switch(t)
	{
		case GFGDataType::HALF_3:
		case GFGDataType::FLOAT_3:
		case GFGDataType::DOUBLE_3:
		case GFGDataType::QUADRUPLE_3:
			return true;
		default:
			return false;
			break;
	}
}

bool GFGPosition::ConvertData(uint8_t data[], size_t dataSize,
							  const double pos[3], GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_3:
			GFGConversions::DoubleToHalfV(data, dataSize, pos, 3);
			break;
		case GFGDataType::FLOAT_3:
			GFGConversions::DoubleToFloatV(data, dataSize, pos, 3);
			break;
		case GFGDataType::DOUBLE_3:
		{
			assert(dataSize >= sizeof(double) * 3);
			std::memcpy(data, pos, sizeof(double) * 3);
			break;
		}
		case GFGDataType::QUADRUPLE_3:
			return false;
		default:
			return false;
	}
	return true;
}

bool GFGPosition::UnConvertData(double pos[3], size_t dataSize,
								const uint8_t data[], GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_3:
			GFGConversions::HalfToDoubleV(pos, dataSize, data, 3);
			break;
		case GFGDataType::FLOAT_3:
			GFGConversions::FloatToDoubleV(pos, dataSize, data, 3);
			break;
		case GFGDataType::DOUBLE_3:
		{
			assert(dataSize >= sizeof(double) * 3);
			std::memcpy(pos, data, sizeof(double) * 3);
			break;
		}
		case GFGDataType::QUADRUPLE_3:
			return false;
		default:
			return false;
	}
	return true;
}

bool GFGNormal::IsCompatible(GFGDataType t)
{
	switch(t)
	{
		case GFGDataType::HALF_2:
		case GFGDataType::HALF_3:
		case GFGDataType::FLOAT_2:
		case GFGDataType::FLOAT_3:
		case GFGDataType::DOUBLE_2:
		case GFGDataType::DOUBLE_3:
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::NORM8_2:
		case GFGDataType::NORM8_3:
		case GFGDataType::NORM16_2:
		case GFGDataType::NORM16_3:
		case GFGDataType::NORM32_2:
		case GFGDataType::NORM32_3:
		case GFGDataType::NORM_2_10_10_10:
		case GFGDataType::CUSTOM_1_15N_16N:
			return true;
		default:
			return false;
	}
}

bool GFGNormal::ConvertData(uint8_t data[], size_t dataSize,
							const double normal[3],
							GFGDataType type,
							const double tangent[3],
							const double bitangent[3])
{
	switch(type)
	{
		case GFGDataType::HALF_2:
			GFGConversions::DoubleToHalfV(data, dataSize, normal, 2);
			break;
		case GFGDataType::HALF_3:
			GFGConversions::DoubleToHalfV(data, dataSize, normal, 3);
			break;
		case GFGDataType::FLOAT_2:
			GFGConversions::DoubleToFloatV(data, dataSize, normal, 2);
			break;
		case GFGDataType::FLOAT_3:
			GFGConversions::DoubleToFloatV(data, dataSize, normal, 3);
			break;
		case GFGDataType::DOUBLE_2:
		{
			assert(dataSize >= sizeof(double) * 2);
			std::memcpy(data, normal, sizeof(double) * 2);
		}
		case GFGDataType::DOUBLE_3:
		{
			assert(dataSize >= sizeof(double) * 3);
			std::memcpy(data, normal, sizeof(double) * 3);
			break;
		}
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::QUADRUPLE_4:
			return false;
		case GFGDataType::NORM8_2:
			GFGConversions::DoubleToNorm8V(data, dataSize, normal, 2);
			break;
		case GFGDataType::NORM8_3:
			GFGConversions::DoubleToNorm8V(data, dataSize, normal, 3);
			break;
		case GFGDataType::NORM16_2:
			GFGConversions::DoubleToNorm16V(data, dataSize, normal, 2);
			break;
		case GFGDataType::NORM16_3:
			GFGConversions::DoubleToNorm16V(data, dataSize, normal, 3);
			break;
		case GFGDataType::NORM32_2:
			GFGConversions::DoubleToNorm32V(data, dataSize, normal, 2);
			break;
		case GFGDataType::NORM32_3:
			GFGConversions::DoubleToNorm32V(data, dataSize, normal, 3);
			break;
		case GFGDataType::NORM_2_10_10_10:
		{
			assert(dataSize >= sizeof(uint32_t));
			uint32_t temp;
			double expandNormal[4];
			std::memcpy(expandNormal, normal, sizeof(double) * 3);
			expandNormal[3] = 0.0;
			temp = GFGConversions::DoublesToInt2_10_10_10(expandNormal);
			std::memcpy(data, &temp, sizeof(uint32_t));
			break;
		}
		case GFGDataType::CUSTOM_1_15N_16N:
		{
			assert(dataSize >= sizeof(uint32_t));
			uint32_t temp;
			temp = GFGConversions::DoublesToCustom_1_15N_16N(normal);
			std::memcpy(data, &temp, sizeof(uint32_t));
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGNormal::UnConvertData(double normal[3], size_t dataSize,
							const uint8_t data[],
							GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_2:
			GFGConversions::HalfToDoubleV(normal, dataSize, data, 2);
			break;
		case GFGDataType::HALF_3:
			GFGConversions::HalfToDoubleV(normal, dataSize, data, 3);
			break;
		case GFGDataType::FLOAT_2:
			GFGConversions::FloatToDoubleV(normal, dataSize, data, 2);
			break;
		case GFGDataType::FLOAT_3:
			GFGConversions::FloatToDoubleV(normal, dataSize, data, 3);
			break;
		case GFGDataType::DOUBLE_2:
		{
			assert(dataSize >= sizeof(double) * 2);
			std::memcpy(normal, data, sizeof(double) * 2);
		}
		case GFGDataType::DOUBLE_3:
		{
			assert(dataSize >= sizeof(double) * 3);
			std::memcpy(normal, data, sizeof(double) * 3);
			break;
		}
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::QUADRUPLE_4:
			return false;
		case GFGDataType::NORM8_2:
			GFGConversions::Norm8ToDoubleV(normal, dataSize, data, 2);
			break;
		case GFGDataType::NORM8_3:
			GFGConversions::Norm8ToDoubleV(normal, dataSize, data, 3);
			break;
		case GFGDataType::NORM16_2:
			GFGConversions::Norm16ToDoubleV(normal, dataSize, data, 2);
			break;
		case GFGDataType::NORM16_3:
			GFGConversions::Norm16ToDoubleV(normal, dataSize, data, 3);
			break;
		case GFGDataType::NORM32_2:
			GFGConversions::Norm32ToDoubleV(normal, dataSize, data, 2);
			break;
		case GFGDataType::NORM32_3:
			GFGConversions::Norm32ToDoubleV(normal, dataSize, data, 3);
			break;
		case GFGDataType::NORM_2_10_10_10:
		{
			assert(dataSize >= sizeof(uint32_t));
			uint32_t temp;
			double normalExpand[4];
			std::memcpy(&temp, data, sizeof(uint32_t));
			GFGConversions::Int2_10_10_10ToDoubles(normalExpand, temp);
			normal[0] = normalExpand[0];
			normal[1] = normalExpand[1];
			normal[2] = normalExpand[2];
			break;
		}
		case GFGDataType::CUSTOM_1_15N_16N:
		{
			assert(dataSize >= sizeof(uint32_t));
			uint32_t temp;
			std::memcpy(&temp, data, sizeof(uint32_t));
			double normalOut[3];
			GFGConversions::Custom_1_15N_16NToDoubles(normal, temp);
			normal[0] = normalOut[0];
			normal[1] = normalOut[1];
			normal[2] = normalOut[2];
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGTangent::IsCompatible(GFGDataType t)
{
	return GFGNormal::IsCompatible(t);
}

bool GFGTangent::ConvertData(uint8_t data[], size_t dataSize,
							 const double tangent[3],
							 GFGDataType type,
							 const double normal[3],
							 const double bitangent[3])
{
	switch(type)
	{
		case GFGDataType::HALF_2:
		case GFGDataType::HALF_3:
		case GFGDataType::FLOAT_2:
		case GFGDataType::FLOAT_3:
		case GFGDataType::DOUBLE_2:
		case GFGDataType::DOUBLE_3:
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::NORM8_2:
		case GFGDataType::NORM8_3:
		case GFGDataType::NORM16_2:
		case GFGDataType::NORM16_3:
		case GFGDataType::NORM32_2:
		case GFGDataType::NORM32_3:
		case GFGDataType::NORM_2_10_10_10:
		case GFGDataType::CUSTOM_1_15N_16N:
			return GFGNormal::ConvertData(data, dataSize,
										  tangent, type,
										  tangent, tangent);
		case GFGDataType::CUSTOM_TANG_H_2N:
			uint32_t result[3];
			GFGConversions::DoublesToCustom_Tang_H_2N(result, normal, tangent, bitangent);
			std::memcpy(data, result, sizeof(uint32_t) * 3);
			break;
		default:
			return false;
	}
	return true;
}

bool GFGBinormal::IsCompatible(GFGDataType t)
{
	return GFGNormal::IsCompatible(t);
}

bool GFGBinormal::ConvertData(uint8_t data[], size_t dataSize,
							  const double bitangent[3],
							  GFGDataType type,
							  const double tangent[3],
							  const double normal[3])
{
	switch(type)
	{
		case GFGDataType::HALF_2:
		case GFGDataType::HALF_3:
		case GFGDataType::FLOAT_2:
		case GFGDataType::FLOAT_3:
		case GFGDataType::DOUBLE_2:
		case GFGDataType::DOUBLE_3:
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::NORM8_2:
		case GFGDataType::NORM8_3:
		case GFGDataType::NORM16_2:
		case GFGDataType::NORM16_3:
		case GFGDataType::NORM32_2:
		case GFGDataType::NORM32_3:
		case GFGDataType::NORM_2_10_10_10:
		case GFGDataType::CUSTOM_1_15N_16N:
			return GFGNormal::ConvertData(data, dataSize,
										  bitangent, type,
										  bitangent, bitangent);
		default:
			return false;
	}
}

bool GFGUV::IsCompatible(GFGDataType t)
{
	switch(t)
	{
		case GFGDataType::HALF_2:
		case GFGDataType::FLOAT_2:
		case GFGDataType::DOUBLE_2:
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::NORM8_2:
		case GFGDataType::NORM16_2:
		case GFGDataType::NORM32_2:
		case GFGDataType::UNORM8_2:
		case GFGDataType::UNORM16_2:
		case GFGDataType::UNORM32_2:
		case GFGDataType::NORM_2_10_10_10:
		case GFGDataType::UNORM_2_10_10_10:
			return true;
		default:
			return false;
	}
}

bool GFGUV::ConvertData(uint8_t data[], size_t dataSize,
						const double uv[2],
						GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_2:
			GFGConversions::DoubleToHalfV(data, dataSize, uv, 2);
			break;
		case GFGDataType::FLOAT_2:
			GFGConversions::DoubleToFloatV(data, dataSize, uv, 2);
			break;
		case GFGDataType::DOUBLE_2:
		{
			assert(dataSize >= sizeof(double) * 2);
			std::memcpy(data, uv, sizeof(double) * 2);
			break;
		}
		case GFGDataType::QUADRUPLE_2:
			return false;
		case GFGDataType::NORM8_2:
			GFGConversions::DoubleToNorm8V(data, dataSize, uv, 2);
			break;
		case GFGDataType::NORM16_2:
			GFGConversions::DoubleToNorm16V(data, dataSize, uv, 2);
			break;
		case GFGDataType::NORM32_2:
			GFGConversions::DoubleToNorm32V(data, dataSize, uv, 2);
			break;
		case GFGDataType::UNORM8_2:
			GFGConversions::DoubleToUnorm8V(data, dataSize, uv, 2);
			break;
		case GFGDataType::UNORM16_2:
			GFGConversions::DoubleToUnorm16V(data, dataSize, uv, 2);
			break;
		case GFGDataType::UNORM32_2:
			GFGConversions::DoubleToUnorm32V(data, dataSize, uv, 2);
			break;
		case GFGDataType::NORM_2_10_10_10:
		{
			double temp[4];
			assert(dataSize >= sizeof(int32_t));
			std::memcpy(temp, data, sizeof(double) * 2);
			temp[2] = 0.0;
			temp[3] = 0.0;
			int32_t result = GFGConversions::DoublesToInt2_10_10_10(temp);
			std::memcpy(data, &result, sizeof(int32_t));
			break;
		}
		case GFGDataType::UNORM_2_10_10_10:
		{
			double temp[4];
			assert(dataSize >= sizeof(uint32_t));
			std::memcpy(temp, data, sizeof(double) * 2);
			temp[2] = 0.0;
			temp[3] = 0.0;
			uint32_t result = GFGConversions::DoublesToUInt2_10_10_10(temp);
			std::memcpy(data, &result, sizeof(uint32_t));
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGUV::UnConvertData(double uv[2], size_t dataSize,
						  const uint8_t data[], GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_2:
			GFGConversions::HalfToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::FLOAT_2:
			GFGConversions::FloatToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::DOUBLE_2:
		{
			assert(dataSize >= sizeof(double) * 2);
			std::memcpy(uv, data, sizeof(double) * 2);
			break;
		}
		case GFGDataType::QUADRUPLE_2:
			return false;
		case GFGDataType::NORM8_2:
			GFGConversions::Norm8ToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::NORM16_2:
			GFGConversions::Norm16ToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::NORM32_2:
			GFGConversions::Norm32ToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::UNORM8_2:
			GFGConversions::UNorm8ToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::UNORM16_2:
			GFGConversions::UNorm16ToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::UNORM32_2:
			GFGConversions::UNorm32ToDoubleV(uv, dataSize, data, 2);
			break;
		case GFGDataType::NORM_2_10_10_10:
		{
			double uvTemp[4];
			int32_t temp;
			assert(dataSize >= sizeof(int32_t));
			std::memcpy(&temp, data, sizeof(int32_t));
			GFGConversions::Int2_10_10_10ToDoubles(uvTemp, temp);
			std::memcpy(uv, uvTemp, sizeof(double) * 2);
			break;
		}
		case GFGDataType::UNORM_2_10_10_10:
		{
			double uvTemp[4];
			uint32_t temp;
			assert(dataSize >= sizeof(uint32_t));
			std::memcpy(&temp, data, sizeof(uint32_t));
			GFGConversions::UInt2_10_10_10ToDoubles(uvTemp, temp);
			std::memcpy(uv, uvTemp, sizeof(double) * 2);
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGWeight::IsCompatible(GFGDataType t, unsigned int maxWeightInfluence)
{
	switch(t)
	{
		case GFGDataType::HALF_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::HALF_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::HALF_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::HALF_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::FLOAT_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::FLOAT_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::FLOAT_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::FLOAT_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::DOUBLE_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::DOUBLE_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::DOUBLE_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::DOUBLE_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::QUADRUPLE_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::QUADRUPLE_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::QUADRUPLE_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::QUADRUPLE_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UNORM8_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::UNORM8_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::UNORM8_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::UNORM8_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UNORM16_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::UNORM16_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::UNORM16_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::UNORM16_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UNORM32_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::UNORM32_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::UNORM32_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::UNORM32_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UNORM16_2_4:
			return maxWeightInfluence <= 8;
		case GFGDataType::UNORM8_4_4:
			return maxWeightInfluence <= 16;
		default:
			return false;
	}
}

bool GFGWeight::ConvertData(uint8_t data[], size_t dataSize,
							const double weight[],
							unsigned int maxWeightInfluence,
							GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::DoubleToHalfV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::HALF_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::DoubleToHalfV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::HALF_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::DoubleToHalfV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::HALF_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::DoubleToHalfV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::DoubleToFloatV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::DoubleToFloatV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::DoubleToFloatV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::DoubleToFloatV(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_1:
		{
			assert(maxWeightInfluence <= 1);
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(data, weight, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_2:
		{
			assert(maxWeightInfluence <= 2);
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(data, weight, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_3:
		{
			assert(maxWeightInfluence <= 3);
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(data, weight, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_4:
		{
			assert(maxWeightInfluence <= 4);
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(data, weight, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::QUADRUPLE_1:
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::QUADRUPLE_4:
			return false;
		case GFGDataType::UNORM8_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::DoubleToUnorm8V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::DoubleToUnorm8V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::DoubleToUnorm8V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::DoubleToUnorm8V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::DoubleToUnorm16V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::DoubleToUnorm16V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::DoubleToUnorm16V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::DoubleToUnorm16V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::DoubleToUnorm32V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::DoubleToUnorm32V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::DoubleToUnorm32V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::DoubleToUnorm32V(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_2_4:
		{
			assert(maxWeightInfluence <= 8);
			GFGConversions::DoubleToUnorm16_2_4(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_4_4:
		{
			assert(maxWeightInfluence <= 16);
			GFGConversions::DoubleToUnorm8_4_4(data, dataSize, weight, maxWeightInfluence);
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGWeight::UnConvertData(double weight[],
							  unsigned int &maxWeightInfluence,
							  size_t dataSize,
							  const uint8_t data[],
							  GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::HalfToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::HALF_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::HalfToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::HALF_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::HalfToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::HALF_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::HalfToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::FloatToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::FloatToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::FloatToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::FLOAT_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::FloatToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_1:
		{
			maxWeightInfluence = 1;
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(weight, data, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_2:
		{
			maxWeightInfluence = 2;
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(weight, data, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_3:
		{
			maxWeightInfluence = 3;
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(weight, data, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::DOUBLE_4:
		{
			maxWeightInfluence = 4;
			assert(dataSize >= sizeof(double) * maxWeightInfluence);
			std::memcpy(weight, data, sizeof(double) * maxWeightInfluence);
			break;
		}
		case GFGDataType::QUADRUPLE_1:
		case GFGDataType::QUADRUPLE_2:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::QUADRUPLE_4:
			return false;
		case GFGDataType::UNORM8_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::UNorm8ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::UNorm8ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::UNorm8ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM8_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::UNorm8ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::UNorm16ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::UNorm16ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::UNorm16ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::UNorm16ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::UNorm32ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::UNorm32ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::UNorm32ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM32_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::UNorm32ToDoubleV(weight, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM16_2_4:
		{
			maxWeightInfluence = 8;
			GFGConversions::Unorm16_2_4ToDoubles(weight, maxWeightInfluence, dataSize, data);
			break;
		}
		case GFGDataType::UNORM8_4_4:
		{
			maxWeightInfluence = 16;
			GFGConversions::Unorm8_4_4ToDoubles(weight, maxWeightInfluence, dataSize, data);
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGWeightIndex::IsCompatible(GFGDataType t, unsigned int maxWeightInfluence)
{
	switch(t)
	{
		case GFGDataType::UINT8_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::UINT8_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::UINT8_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::UINT8_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UINT16_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::UINT16_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::UINT16_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::UINT16_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UINT32_1:
			return maxWeightInfluence <= 1;
		case GFGDataType::UINT32_2:
			return maxWeightInfluence <= 2;
		case GFGDataType::UINT32_3:
			return maxWeightInfluence <= 3;
		case GFGDataType::UINT32_4:
			return maxWeightInfluence <= 4;
		case GFGDataType::UNORM_2_10_10_10:
			return maxWeightInfluence <= 3;
		case GFGDataType::UINT16_2_4:
			return maxWeightInfluence <= 8;
		case GFGDataType::UINT8_4_4:
			return maxWeightInfluence <= 16;
		default:
			return false;
	}
}

bool GFGWeightIndex::ConvertData(uint8_t data[], size_t dataSize,
								 const unsigned int wIndex[],
								 unsigned int maxWeightInfluence,
								 GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::UINT8_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::UIntToUInt8V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::UIntToUInt8V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::UIntToUInt8V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::UIntToUInt8V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::UIntToUInt16V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::UIntToUInt16V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::UIntToUInt16V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::UIntToUInt16V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_1:
		{
			assert(maxWeightInfluence <= 1);
			GFGConversions::UIntToUInt32V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_2:
		{
			assert(maxWeightInfluence <= 2);
			GFGConversions::UIntToUInt32V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_3:
		{
			assert(maxWeightInfluence <= 3);
			GFGConversions::UIntToUInt32V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_4:
		{
			assert(maxWeightInfluence <= 4);
			GFGConversions::UIntToUInt32V(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM_2_10_10_10:
		{
			assert(maxWeightInfluence <= 3);
			assert(dataSize >= sizeof(uint32_t));
			uint32_t temp = GFGConversions::UIntsToUInt2_10_10_10(wIndex);
			std::memcpy(data, &temp, sizeof(uint32_t));
			break;
		}
		case GFGDataType::UINT16_2_4:
		{
			assert(maxWeightInfluence <= 8);
			GFGConversions::UIntToUInt16_2_4(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_4_4:
		{
			assert(maxWeightInfluence <= 16);
			GFGConversions::UIntToUInt8_4_4(data, dataSize, wIndex, maxWeightInfluence);
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGWeightIndex::UnConvertData(unsigned int wIndex[],
								   unsigned int &maxWeightInfluence,
								   size_t dataSize,
								   const uint8_t data[],
								   GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::UINT8_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::UInt8ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::UInt8ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::UInt8ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT8_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::UInt8ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::UInt16ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::UInt16ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::UInt16ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT16_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::UInt16ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_1:
		{
			maxWeightInfluence = 1;
			GFGConversions::UInt32ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_2:
		{
			maxWeightInfluence = 2;
			GFGConversions::UInt32ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_3:
		{
			maxWeightInfluence = 3;
			GFGConversions::UInt32ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UINT32_4:
		{
			maxWeightInfluence = 4;
			GFGConversions::UInt32ToUIntV(wIndex, dataSize, data, maxWeightInfluence);
			break;
		}
		case GFGDataType::UNORM_2_10_10_10:
		{;
			uint32_t temp;
			std::memcpy(&temp, &data, sizeof(uint32_t));
			maxWeightInfluence = 3;
			GFGConversions::UInt2_10_10_10ToUInts(wIndex, temp);
			break;
		}
		case GFGDataType::UINT16_2_4:
		{
			maxWeightInfluence = 8;
			GFGConversions::UInt16_2_4ToUInts(wIndex, maxWeightInfluence, dataSize, data);
			break;
		}
		case GFGDataType::UINT8_4_4:
		{
			maxWeightInfluence = 16;
			GFGConversions::UInt8_4_4ToUInts(wIndex, maxWeightInfluence, dataSize, data);
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGColor::IsCompatible(GFGDataType t)
{
	switch(t)
	{
		case GFGDataType::HALF_3:
		case GFGDataType::FLOAT_3:
		case GFGDataType::DOUBLE_3:
		case GFGDataType::QUADRUPLE_3:
		case GFGDataType::UNORM8_3:
		case GFGDataType::UNORM16_3:
		case GFGDataType::UNORM32_3:
		case GFGDataType::UNORM_2_10_10_10:
		case GFGDataType::UINT_10F_11F_11F:
			return true;
		default:
			return false;
	}
}

bool GFGColor::ConvertData(uint8_t data[], size_t dataSize,
							const double color[3],
							GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_3:
			GFGConversions::DoubleToHalfV(data, dataSize, color, 3);
			break;
		case GFGDataType::FLOAT_3:
			GFGConversions::DoubleToFloatV(data, dataSize, color, 3);
			break;
		case GFGDataType::DOUBLE_3:
		{
			assert(dataSize >= sizeof(double) * 3);
			std::memcpy(data, color, sizeof(double) * 3);
			break;
		}
		case GFGDataType::QUADRUPLE_3:
			return false;
		case GFGDataType::UNORM8_3:
			GFGConversions::DoubleToUnorm8V(data, dataSize, color, 3);
			break;
		case GFGDataType::UNORM16_3:
			GFGConversions::DoubleToUnorm16V(data, dataSize, color, 3);
			break;
		case GFGDataType::UNORM32_3:
			GFGConversions::DoubleToUnorm32V(data, dataSize, color, 3);
			break;
		case GFGDataType::UNORM_2_10_10_10:
		{
			assert(dataSize >= sizeof(uint32_t));
			double tempIn[4];
			std::memcpy(tempIn, color, sizeof(double) * 3);
			tempIn[3] = 0.0;
			uint32_t temp = GFGConversions::DoublesToUInt2_10_10_10(tempIn);
			std::memmove(data, &temp, sizeof(uint32_t));
			break;
		}
		case GFGDataType::UINT_10F_11F_11F:
		{
			uint32_t temp;
			assert(dataSize >= sizeof(uint32_t));
			temp = GFGConversions::DoublesToUInt10F_11F_11F(color);
			std::memcpy(data, &temp, sizeof(uint32_t));
			break;
		}
		default:
			return false;
	}
	return true;
}

bool GFGColor::UnConvertData(double color[3], size_t dataSize,
							 const uint8_t data[],
							 GFGDataType type)
{
	switch(type)
	{
		case GFGDataType::HALF_3:
			GFGConversions::HalfToDoubleV(color, dataSize, data, 3);
			break;
		case GFGDataType::FLOAT_3:
			GFGConversions::FloatToDoubleV(color, dataSize, data, 3);
			break;
		case GFGDataType::DOUBLE_3:
		{
			assert(dataSize >= sizeof(double) * 3);
			std::memcpy(color, data, sizeof(double) * 3);
			break;
		}
		case GFGDataType::QUADRUPLE_3:
			return false;
		case GFGDataType::UNORM8_3:
			GFGConversions::UNorm8ToDoubleV(color, dataSize, data, 3);
			break;
		case GFGDataType::UNORM16_3:
			GFGConversions::UNorm16ToDoubleV(color, dataSize, data, 3);
			break;
		case GFGDataType::UNORM32_3:
			GFGConversions::UNorm32ToDoubleV(color, dataSize, data, 3);
			break;
		case GFGDataType::UNORM_2_10_10_10:
		{
			assert(dataSize >= sizeof(uint32_t));
			double tempIn[4];
			uint32_t temp;
			std::memcpy(&temp, data, sizeof(uint32_t));
			GFGConversions::UInt2_10_10_10ToDoubles(tempIn, temp);
			color[0] = tempIn[0];
			color[1] = tempIn[1];
			color[2] = tempIn[2];
			break;
		}
		case GFGDataType::UINT_10F_11F_11F:
		{
			uint32_t colorData;
			std::memcpy(&colorData, data, sizeof(uint32_t));
			GFGConversions::UInt10F_11F_11FToDoubles(color, colorData);
			break;
		}
		default:
			return false;
	}
	return true;
}